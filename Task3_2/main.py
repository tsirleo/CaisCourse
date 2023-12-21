import json
from copy import deepcopy

# x86 General Purpose Registers https://www.tortall.net/projects/yasm/manual/html/arch-x86-registers.html
registers_64_lst = ["rax", "rbx", "rcx", "rdx", "rsp", "rbp", "rsi", "rdi", "rflags", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"]
registers_64 = {"rax", "rbx", "rcx", "rdx", "rsp", "rbp", "rsi", "rdi", "rflags", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"}
registers_32 = {"eax", "ebx", "ecx", "edx", "esp", "ebp", "esi", "edi", "eflags", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"}
registers_16 = {"ax", "bx", "cx", "dx", "sp", "bp", "si", "di", "flags", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"}
registers_8l = {"al", "bl", "cl", "dl", "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"}
registers_8h = {"ah", "bh", "ch", "dh"}
registers_union = registers_64.union(registers_8h, registers_8l, registers_16, registers_32)


def get_key_and_bytemask(register):
    if register in registers_8h:
        bytemask = 0b00000010
    elif register in registers_8l:
        bytemask = 0b00000001
    elif register in registers_16:
        bytemask = 0b00000011
    elif register in registers_32:
        bytemask = 0b00001111
    else:
        bytemask = 0b11111111

    if register[0] == "r":
        if register[-1] == "b" or register[-1] == "w" or register[-1] == "d":
            key = register[:-1]
        else:
            key = register
    elif register[0] == "e":
        key = "r" + register[1:]
    elif bytemask == 0b00000011:
        key = "r" + register
    elif register[0] in ["a", "b", "c", "d"]:
        key = "r" + register[0] + "x"
    else:
        key = "r" + register[:-1]

    return key, bytemask


def is_marked_memory_segment(memory_seg, memory_taints):
    for mem_t in memory_taints:
        if mem_t[0] <= memory_seg[0] <= mem_t[0] + mem_t[1] - 1:
            return True

        if mem_t[0] <= memory_seg[0] + memory_seg[1] - 1 <= mem_t[0] + mem_t[1] - 1:
            return True

        if mem_t[0] >= memory_seg[0] and mem_t[0] + mem_t[1] - 1 <= memory_seg[0] + memory_seg[1] - 1:
            return True

    return False


def mark_memory_segment(memory_seg, memory_taints):
    start = None
    end = None
    deleted_indices = []

    for i in range(len(memory_taints)):
        if memory_taints[i][0] <= memory_seg[0] <= memory_taints[i][0] + memory_taints[i][1]:
            start = i

        if memory_taints[i][0] - 1 <= memory_seg[0] + memory_seg[1] - 1 <= memory_taints[i][0] + memory_taints[i][1] - 1:
            end = i

        if memory_taints[i][0] > memory_seg[0] and memory_taints[i][0] + memory_taints[i][1] - 1 < memory_seg[0] + memory_seg[1] - 1:
            deleted_indices.append(i)

    if start == end and start is not None:
        return

    if start is None and end is None:
        for ind in reversed(deleted_indices):
            del memory_taints[ind]

        memory_taints.append(memory_seg)
    elif start is not None and end is not None:
        deleted_indices.append(end)
        memory_taints[start] = [memory_taints[start][0], memory_taints[end][0] - memory_taints[start][0] + memory_taints[end][1]]

        for ind in sorted(deleted_indices, reverse=True):
            del memory_taints[ind]
    elif start is None:
        memory_taints[end] = [memory_seg[0], memory_taints[end][1] + memory_taints[end][0] - memory_seg[0]]

        for ind in reversed(deleted_indices):
            del memory_taints[ind]
    else:
        memory_taints[start] = [memory_taints[start][0], (memory_seg[0] + memory_seg[1] - 1) - memory_taints[start][0] + 1]

        for ind in reversed(deleted_indices):
            del memory_taints[ind]


def unmark_memory_segment(memory_seg, memory_taints):
    start = None
    end = None
    deleted_indices = []

    for i in range(len(memory_taints)):
        if memory_taints[i][0] < memory_seg[0] < memory_taints[i][0] + memory_taints[i][1] - 1:
            start = i

        if memory_taints[i][0] < memory_seg[0] + memory_seg[1] - 1 < memory_taints[i][0] + memory_taints[i][1] - 1:
            end = i

        if memory_taints[i][0] >= memory_seg[0] and memory_taints[i][0] + memory_taints[i][1] - 1 <= memory_seg[0] + memory_seg[1] - 1:
            deleted_indices.append(i)

    if start == end and start is not None:
        prev_end = deepcopy(memory_taints[end])
        memory_taints[start][1] = memory_seg[0] - memory_taints[start][0]
        memory_taints.append([(memory_seg[0] + memory_seg[1] - 1) + 1, prev_end[0] + prev_end[1] - 1 - (memory_seg[0] + memory_seg[1] - 1)])
    elif start is None and end is None:
        for ind in reversed(deleted_indices):
            del memory_taints[ind]
    else:
        if start is not None:
            memory_taints[start][1] = memory_seg[0] - memory_taints[start][0]

        if end is not None:
            memory_taints[end] = [(memory_seg[0] + memory_seg[1] - 1) + 1, memory_taints[end][0] + memory_taints[end][1] - 1 - (memory_seg[0] + memory_seg[1] - 1)]

        for ind in reversed(deleted_indices):
            del memory_taints[ind]


def mark_register(register, register_taints):
    key, mask = get_key_and_bytemask(register)
    register_taints[key] |= mask


def unmark_register(register, register_taints):
    key, mask = get_key_and_bytemask(register)
    register_taints[key] &= ~mask


def update_taints(instruction, register_taints, memory_taints):
    for register in instruction["readRegs"]:
        if register in registers_union:
            key, mask = get_key_and_bytemask(register)
            if register_taints[key] & mask != 0b0:
                mark = True
                break
    else:
        mark = False

    if not mark:
        for mem in instruction["readMem"]:
            if is_marked_memory_segment(mem, memory_taints):
                mark = True
                break

    if mark:
        for register in instruction["writeRegs"]:
            if register in registers_union:
                mark_register(register, register_taints)

        for mem in instruction["writtenMem"]:
            mark_memory_segment(mem, memory_taints)
    else:
        if instruction["text"].startswith("mov") or instruction["text"].startswith("cmov"):
            for register in instruction["writeRegs"]:
                if register in registers_union:
                    unmark_register(register, register_taints)

            for mem in instruction["writtenMem"]:
                unmark_memory_segment(mem, memory_taints)


def get_marked_register_info(register, register_mask):
    marks = []
    start = 8

    if register_mask == 0b00000000:
        return []

    if register_mask == 0b11111111:
        return [register]

    if register_mask & 0b11110000 != 0b0:
        start = 4

    if register_mask & 0b00001111 == 0b00001111:
        marks.append("e" + register[1:] if register[-1].isalpha() else register + "d")
    else:
        start = 2
        if register_mask & 0b00000011 == 0b00000011:
            marks.append(register[1:] if register[-1].isalpha() else register + "w")
        else:
            if register == "rip" or register == "rflags":
                start = 0
            elif register in ["rax", "rbx", "rcx", "rdx"]:
                if register_mask & 0b00000010:
                    marks.append(register[1] + "h")

                if register_mask & 0b00000001:
                    marks.append(register[1] + "l")
            else:
                start = 1
                if register_mask & 0b00000001:
                    marks.append(register[1:] + "l" if register[-1].isalpha() else register + "b")

    new_mark = []
    while start < 8:
        if not (register_mask & (1 << start)):
            if len(new_mark) > 0:
                marks.append(deepcopy(new_mark))
                new_mark = []

            start += 1
        else:
            if len(new_mark) > 0:
                new_mark[2] = new_mark[2] + 1
            else:
                new_mark = [register, start, 1]

            start += 1

    if len(new_mark) > 0:
        marks.append(deepcopy(new_mark))

    return marks


def add_taints(source, register_taints, memory_taints):
    for taint in source:
        if isinstance(taint, str) and taint in registers_union:
            mark_register(taint, register_taints)
        elif isinstance(taint, list) and isinstance(taint[0], int):
            mark_memory_segment(taint, memory_taints)


def save_taints(step, register_taints, memory_taints, result):
    info = {"step": step, "answer": []}

    for reg in register_taints.keys():
        info["answer"].extend(get_marked_register_info(reg, register_taints[reg]))

    # info["answer"] = sorted(info["answer"], key=lambda item: len(item), reverse=True)
    info["answer"].extend(sorted(deepcopy(memory_taints), key=lambda item: item[0]))

    result.append(info)


def process_data(trace, tests):
    register_taints = {}

    for register in registers_64_lst:
        register_taints[register] = 0b00000000

    memory_taints = []
    result = []
    i = 0

    for command in tests:
        while i < command["step"]:
            update_taints(trace[i], register_taints, memory_taints)
            i += 1

        match command["type"]:
            case "source":
                add_taints(command["taint"], register_taints, memory_taints)
            case "sink":
                save_taints(command["step"], register_taints, memory_taints, result)

        # for python < 3.10
        # if command["type"] == "source":
        #     add_taints(command["taint"], register_taints, memory_taints)
        # elif command["type"] == "sink":
        #     save_taints(command["step"], register_taints, memory_taints, result)

    return result


def load_data(input_filename, tests_filename):
    with open(input_filename) as trace_file:
        trace = json.load(trace_file)

    with open(tests_filename) as test_file:
        tests = json.load(test_file)

    return trace, tests


def write_output(output_filename, result):
    with open(output_filename, 'w') as output_file:
        if len(result) > 0:
            output_file.write("[\n")
            for line in result[:-1]:
                output_file.write(f"    {json.dumps(line)},\n")
            output_file.write(f"    {json.dumps(result[-1])}\n")
            output_file.write("]")
        else:
            output_file.write("[]")


def main(input_filename, tests_filename, output_filename):
    trace, tests = load_data(input_filename, tests_filename)
    result = process_data(trace, tests)
    write_output(output_filename, result)


if __name__ == '__main__':
    import sys

    if len(sys.argv) != 4:
        print("Usage: python3 main.py input.json tests.json output.json")
    else:
        main(sys.argv[1], sys.argv[2], sys.argv[3])
