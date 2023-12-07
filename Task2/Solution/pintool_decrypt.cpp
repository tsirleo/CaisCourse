#include <pin.H>
#include <iostream>
#include <fstream>
using namespace std;

VOID OnBFSetKey(UINT32 len, const unsigned char *data) {
    ifstream keyFile("decrypt.key", ios::in | ios::binary);
    if (keyFile.is_open()) {
        keyFile.seekg(0, ios::end);
        int keySize = keyFile.tellg();
        keyFile.seekg(0, ios::beg);

        char* newKey = new char[keySize];
        keyFile.read(newKey, keySize);
        // cout << "The key in decrypt is going to be replased:" << endl << "  old key: " << data << endl << "  new key: " << newKey << endl;
        memcpy(const_cast<unsigned char*>(data), newKey, len);

        // cout << endl << "The key in decrypt is replaced:" << endl << "  key: " << data << endl;
        delete[] newKey;
        keyFile.close();
    } else {
        cerr << "Unable to open the keyfile." << endl;
        exit(1);
    }
}

VOID ImageLoad(IMG img, VOID* v) {
    RTN rtn = RTN_FindByName(img, "BF_set_key");
    if (RTN_Valid(rtn)) {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)OnBFSetKey, IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_FUNCARG_ENTRYPOINT_VALUE, 2, IARG_END);
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
