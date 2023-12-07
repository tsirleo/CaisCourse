/*
 * CAIS Course Assignment 3: Trace Tool
 *
 * © 2017 CAIS Course Authors <https://caiscourse.ru/>.  This file is governed
 * by terms of the MIT license.
 */

#include "pin.H"

KNOB<string> knobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "", "Output file name");
KNOB<BOOL> knobJsonMode(KNOB_MODE_WRITEONCE, "pintool", "j", "0", "Format output as a JSON file [default: text]");
KNOB<BOOL> knobDataflow(KNOB_MODE_WRITEONCE, "pintool", "d", "0", "Emit data flow information [default: false]");

namespace cca3
{
    class TraceTool
    {
        struct InsnContext;
        struct InsnDescriptor;

    public:
        // Enumeration of output modes.
        enum OutputMode
        {
            kTextOutput,
            kJsonOutput,
        };

    public:
        // Construct the TraceTool.
        TraceTool(FILE *outputFile, OutputMode outputMode, bool dataflow) : m_outputFile(outputFile),
            m_outputMode(outputMode), m_dataflow(dataflow), m_isFirstInsn(true)
        {
            // Hook image loading, we need this to isolate main image from the
            // libraries (and especially ld-linux.so and the VDSO).
            IMG_AddInstrumentFunction(instrumentImageTramp, this);

            // Hook individual instructions.
            INS_AddInstrumentFunction(instrumentInsnTramp, this);

            // Print file header.
            switch (m_outputMode) {
                case kTextOutput:
                    outputTextHeader();
                    break;

                case kJsonOutput:
                    outputJsonHeader();
                    break;
            }
        }

        // Destructor: closes the output file.
        ~TraceTool()
        {
            // Print file footer.
            switch (m_outputMode) {
                case kTextOutput:
                    outputTextFooter();
                    break;

                case kJsonOutput:
                    outputJsonFooter();
                    break;
            }

            // Close file.
            fclose(m_outputFile);
        }

    public:
        // Finalize the tool, i.e. call its destructor.
        static void
        finalize(INT32 code, void *self)
        {
            delete static_cast<TraceTool *>(self);
        }

    private:
        // Instrumentation callback: image-level.  This is a trampoline to the
        // actual routine.
        static void
        instrumentImageTramp(IMG img, void *self)
        {
            static_cast<TraceTool *>(self)->instrumentImage(img);
        }

        // Instrumentation callback: image-level.
        void
        instrumentImage(IMG img)
        {
            SEC                         sec;
            SectionDescriptor           d;

            // For main executable, iterate its executable sections (.init, .plt,
            // .text, etc.).
            if (IMG_IsMainExecutable(img)) {
                for (sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)) {
                    if (SEC_Mapped(sec) && SEC_IsExecutable(sec)) {
                        // Don't go to .plt.
                        if (SEC_Name(sec) != ".plt") {
                            // Populate descriptor.
                            d.start     = SEC_Address(sec);
                            d.end       = d.start + SEC_Size(sec);

                            // Store.
                            m_sections.push_back(d);
                        }
                    }
                }
            }
        }

    private:
        // Instrumentation callback: instruction-level.  This is a trampoline to
        // the actual routine.
        static void
        instrumentInsnTramp(INS ins, void *self)
        {
            static_cast<TraceTool *>(self)->instrumentInsn(ins);
        }

        // Instrumentation callback: instruction-level.
        void
        instrumentInsn(INS ins)
        {
            InsnDescriptor              d;
            uint32_t                    i;

            // Start populating the descriptor.
            d.address                   = INS_Address(ins);
            d.size                      = INS_Size(ins);

            // We are not interested in instructions outside of the text sections
            // of the main executable.
            if (addressInSections(d.address)) {
                // Finalize descriptor.
                d.hexDump               = hexDump(reinterpret_cast<const void *>(d.address), d.size);
                d.text                  = INS_Disassemble(ins);
                d.isBranch              = INS_IsBranchOrCall(ins);
                d.isCall                = INS_IsCall(ins);
                d.isReturn              = INS_IsRet(ins);
                d.readRegs              = std::vector<REG>();
                d.writtenRegs           = std::vector<REG>();

                // Store read registers.
                for (i = 0; i != INS_MaxNumRRegs(ins); ++i) d.readRegs.push_back(INS_RegR(ins, i));

                // Store written registers.
                for (i = 0; i != INS_MaxNumWRegs(ins); ++i) d.writtenRegs.push_back(INS_RegW(ins, i));

                // Insert descriptor into instruction map.
                m_insns.insert(std::make_pair(d.address, d));

                // Instrument this one, depending on whether it is a branch.
                if (d.isBranch) {
                    // See if instruction works with memory.
                    if (INS_IsMemoryRead(ins) || INS_IsMemoryWrite(ins)) {
                        // Pass target address; pass memory access.
                        INS_InsertCall(ins, IPOINT_BEFORE, reinterpret_cast<AFUNPTR>(traceInsnTramp),
                            IARG_INST_PTR,
                            IARG_BRANCH_TARGET_ADDR,
                            IARG_MULTI_MEMORYACCESS_EA,
                            IARG_PTR, this,
                            IARG_END
                        );
                    } else {
                        // Pass target address; no memory access, pass null.
                        INS_InsertCall(ins, IPOINT_BEFORE, reinterpret_cast<AFUNPTR>(traceInsnTramp),
                            IARG_INST_PTR,
                            IARG_BRANCH_TARGET_ADDR,
                            IARG_PTR, 0,
                            IARG_PTR, this,
                            IARG_END
                        );
                    }
                } else {
                    // See if instruction works with memory.
                    if (INS_IsMemoryRead(ins) || INS_IsMemoryWrite(ins)) {
                        // No target address, pass zero; pass memory access.
                        INS_InsertCall(ins, IPOINT_BEFORE, reinterpret_cast<AFUNPTR>(traceInsnTramp),
                            IARG_INST_PTR,
                            IARG_ADDRINT, 0,
                            IARG_MULTI_MEMORYACCESS_EA,
                            IARG_PTR, this,
                            IARG_END
                        );
                    } else {
                        // No target address, pass zero; no memory access, pass null.
                        INS_InsertCall(ins, IPOINT_BEFORE, reinterpret_cast<AFUNPTR>(traceInsnTramp),
                            IARG_INST_PTR,
                            IARG_ADDRINT, 0,
                            IARG_PTR, 0,
                            IARG_PTR, this,
                            IARG_END
                        );
                    }
                }
            }
        }

    private:
        // Per-instruction trace callback.  This is a trampoline to the actual
        // routine.
        static void
        traceInsnTramp(ADDRINT address, ADDRINT targetAddress, PIN_MULTI_MEM_ACCESS_INFO *accessInfo, void *self)
        {
            static_cast<TraceTool *>(self)->traceInsn(address, targetAddress, accessInfo);
        }

        // Per-instruction trace callback.
        void
        traceInsn(uintptr_t address, uintptr_t targetAddress, const PIN_MULTI_MEM_ACCESS_INFO *accessInfo)
        {
            const InsnDescriptor *      d;
            InsnContext                 c;
            uint32_t                    i;

            // Grab instruction descriptor and populate context.
            d                           = &m_insns[address];
            c.targetAddress             = targetAddress;
            c.isForeignBranch           = d->isBranch && !d->isReturn && !addressInSections(targetAddress);

            // Prepare access information.
            if (accessInfo) {
                for (i = 0; i != accessInfo->numberOfMemops; ++i) {
                    switch (accessInfo->memop[i].memopType) {
                        case PIN_MEMOP_LOAD:
                            c.readAddresses.push_back(accessInfo->memop[i].memoryAddress);
                            c.readSizes.push_back(accessInfo->memop[i].bytesAccessed);

                            break;

                        case PIN_MEMOP_STORE:
                            c.writtenAddresses.push_back(accessInfo->memop[i].memoryAddress);
                            c.writtenSizes.push_back(accessInfo->memop[i].bytesAccessed);

                            break;
                    }
                }
            }

            // If this is a foreign branch, try to get target name.
            if (c.isForeignBranch) c.foreignTargetName = RTN_FindNameByAddress(targetAddress);

            // Write out the instruction.
            switch (m_outputMode) {
                case kTextOutput:
                    outputTextInsn(d, &c);
                    break;

                case kJsonOutput:
                    outputJsonInsn(d, &c);
                    break;
            }

            // No longer the first instruction.
            m_isFirstInsn = false;
        }

    private:
        // Text mode: print trace header.
        void
        outputTextHeader()
        {
        }

        // Text mode: print trace footer.
        void
        outputTextFooter()
        {
        }

        // Text mode: print out instruction.
        void
        outputTextInsn(const InsnDescriptor *d, const InsnContext *c)
        {
            size_t                      i;

            // Print basic instruction information.
            fprintf(m_outputFile, "[%016lX] %-32s %s\n", d->address, d->hexDump.c_str(), d->text.c_str());

            // Print branch taret information.
            if (d->isBranch) {
                fprintf(m_outputFile, "    ** Branch\n");

                if (c->isForeignBranch) {
                    fprintf(m_outputFile, "    -> [%016lX] = <%s>\n", c->targetAddress, c->foreignTargetName.c_str());
                }
            }

            // If required, print dataflow info.
            if (m_dataflow) {
                // Print register reads.
                for (i = 0; i != d->readRegs.size(); ++i) {
                    fprintf(m_outputFile, "    RR %s\n", REG_StringShort(d->readRegs[i]).c_str());
                }

                // Print register writes.
                for (i = 0; i != d->writtenRegs.size(); ++i) {
                    fprintf(m_outputFile, "    WR %s\n", REG_StringShort(d->writtenRegs[i]).c_str());
                }

                // Print memory reads.
                for (i = 0; i != c->readAddresses.size(); ++i) {
                    fprintf(m_outputFile, "    RM %016lX %zu\n", c->readAddresses[i], c->readSizes[i]);
                }

                // Print memory writes.
                for (i = 0; i != c->writtenAddresses.size(); ++i) {
                    fprintf(m_outputFile, "    WM %016lX %zu\n", c->writtenAddresses[i], c->writtenSizes[i]);
                }
            }
        }

    private:
        // JSON mode: print trace header.
        void
        outputJsonHeader()
        {
            fprintf(m_outputFile, "[");
        }

        // JSON mode: print trace footer.
        void
        outputJsonFooter()
        {
            fprintf(m_outputFile, "\n]\n");
        }

        // JSON mode: print out instructions.
        void
        outputJsonInsn(const InsnDescriptor *d, const InsnContext *c)
        {
            size_t                      i;

            // Print prefix.
            if (m_isFirstInsn) {
                fprintf(m_outputFile, "\n    { ");
            } else {
                fprintf(m_outputFile, ",\n    { ");
            }

            // Print basic instruction information.
            fprintf(m_outputFile, "\"address\": %lu, \"hexDump\": \"%s\", \"text\": \"%s\"", d->address, d->hexDump.c_str(), d->text.c_str());

            // Print branch target information.
            if (d->isBranch) {
                fprintf(m_outputFile, ", \"isBranch\": true");

                if (c->isForeignBranch) {
                    fprintf(m_outputFile, ", \"isForeignBranch\": true, \"foreignTargetAddress\": %lu, \"foreignTargetName\": \"%s\"", c->targetAddress, c->foreignTargetName.c_str());
                }
            }

            // If required, print dataflow info.
            if (m_dataflow) {
                // Print register reads.
                fprintf(m_outputFile, ", \"readRegs\": [ ");

                // Iterate read registers.
                for (i = 0; i != d->readRegs.size(); ++i) {
                    fprintf(m_outputFile, "%s\"%s\"", i ? ", " : "", REG_StringShort(d->readRegs[i]).c_str());
                }

                // Print register writes.
                fprintf(m_outputFile, " ], \"writeRegs\": [ ");

                // Iterate written registers.
                for (i = 0; i != d->writtenRegs.size(); ++i) {
                    fprintf(m_outputFile, "%s\"%s\"", i ? ", " : "", REG_StringShort(d->writtenRegs[i]).c_str());
                }

                // Print memory reads.
                fprintf(m_outputFile, " ], \"readMem\": [ ");

                // Iterate read memory.
                for (i = 0; i != c->readAddresses.size(); ++i) {
                    fprintf(m_outputFile, "%s[ %lu, %zu ]", i ? ", " : "", c->readAddresses[i], c->readSizes[i]);
                }

                // Print memory writes.
                fprintf(m_outputFile, " ], \"writtenMem\": [ ");

                // Iterate written memory.
                for (i = 0; i != c->writtenAddresses.size(); ++i) {
                    fprintf(m_outputFile, "%s[ %lu, %zu ]", i ? ", " : "", c->writtenAddresses[i], c->writtenSizes[i]);
                }

                // Terminate last array.
                fprintf(m_outputFile, " ]");
            }

            // Print suffix.
            fprintf(m_outputFile, " }");
        }

    private:
        // Check whether address is within the section range.
        bool
        addressInSections(uintptr_t address) const
        {
            size_t                      i;

            for (i = 0; i != m_sections.size(); ++i) {
                if ((address >= m_sections[i].start) && (address <= m_sections[i].end)) {
                    return true;
                }
            }

            return false;
        }

        // Make hex dump of a buffer.
        string
        hexDump(const void *address, size_t size)
        {
            static const char           hexTable[] = "0123456789ABCDEF";

            string                      ret;
            const unsigned char *       p;

            for (p = static_cast<const unsigned char *>(address); size--; p++) {
                ret.push_back(hexTable[*p / 16]);
                ret.push_back(hexTable[*p % 16]);
            }

            return ret;
        }

    private:
        FILE *                          m_outputFile;
        OutputMode                      m_outputMode;
        
        bool                            m_dataflow;

        bool                            m_isFirstInsn;

    private:
        // Helper structure: section descriptor.
        struct SectionDescriptor
        {
            uintptr_t                   start;
            uintptr_t                   end;
        };

        // Sections that we need to track.
        std::vector<SectionDescriptor>  m_sections;

    private:
        // Helper structure: instruction descriptor.
        struct InsnDescriptor
        {
            uintptr_t                   address;
            uintptr_t                   size;

            std::string                 hexDump;
            std::string                 text;

            bool                        isBranch;
            bool                        isCall;
            bool                        isReturn;

            std::vector<REG>            readRegs;
            std::vector<REG>            writtenRegs;
        };

        // Instructions that we need to track.
        std::map<uintptr_t, InsnDescriptor>
                                        m_insns;

    private:
        // Helper structure: instruction context.
        struct InsnContext
        {
            uintptr_t                   targetAddress;
            
            bool                        isForeignBranch;
            std::string                 foreignTargetName;

            std::vector<uintptr_t>      readAddresses;
            std::vector<size_t>         readSizes;

            std::vector<uintptr_t>      writtenAddresses;
            std::vector<size_t>         writtenSizes;
        };
    };
}

static int
usage()
{
    fprintf(stderr, "This tool provides the tracer for CAIS course assignment 3.\n\n");
    fprintf(stderr, "%s", KNOB_BASE::StringKnobSummary().c_str());

    return -1;
}

int
main(int argc, char **argv)
{
    FILE *                              outputFile;
    cca3::TraceTool::OutputMode         outputMode(cca3::TraceTool::kTextOutput);

    // Parse command line.
    if (PIN_Init(argc, argv) || knobOutputFile.Value().empty()) return usage();

    // Open file.
    if (NULL == (outputFile = fopen(knobOutputFile.Value().c_str(), "w"))) {
        fprintf(stderr, "Cannot open output file: \"%s\".\n", knobOutputFile.Value().c_str());
        return 1;
    }

    // Decide on output mode.
    if (knobJsonMode) outputMode = cca3::TraceTool::kJsonOutput;

    // Create and configure the tool instance.
    cca3::TraceTool *traceTool = new cca3::TraceTool(outputFile, outputMode, knobDataflow);

    // Hook up destructor to the internal Pin finalization chain and start the
    // program.
    PIN_AddFiniFunction(cca3::TraceTool::finalize, traceTool);
    PIN_InitSymbols();
    PIN_StartProgram();

    // Never reached.
    return 0;
}
