#include <pin.H>
#include <iostream>
using namespace std;

VOID BeforeBFSetKey(UINT32 len, const unsigned char *data) {
    // cout << "BF_set_key called with:" << endl <<"  length: " << len << endl << "  data: " << data << endl;

    FILE *keyFile = fopen("encrypt.key", "wb");
    if (keyFile && fwrite(data, sizeof(char), len, keyFile) == len) {
        // cout << endl << "Data is written in file successfully" << endl;
        fclose(keyFile);
    } else {
        cerr << "Unable to open the keyfile. Or error occured while writting." << endl;
        exit(1);
    }
}

VOID ImageLoad(IMG img, VOID* v) {
    RTN rtn = RTN_FindByName(img, "BF_set_key");
    if (RTN_Valid(rtn)) {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)BeforeBFSetKey, IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_FUNCARG_ENTRYPOINT_VALUE, 2, IARG_END);
        RTN_Close(rtn);
    }
}

int main(int argc, char* argv[]) {
    PIN_InitSymbols();
    if (PIN_Init(argc, argv)) return -1;
    IMG_AddInstrumentFunction(ImageLoad, 0);
    PIN_StartProgram();
    return 0;
}
