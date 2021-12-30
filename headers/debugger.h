#ifndef HADESDBG_DEBUGGER_H
#define HADESDBG_DEBUGGER_H

#include <main_types.h>
#include <fstream>
#include <sys/user.h>
#include <cstdlib>
#include <vector>
#include <regs.h>

using namespace std;

class HadesDbg {
private:
    unsigned long long int fileEntry, effectiveEntry;
    unsigned char* replacedFileEntry;
    pid_t pid;
    int memoryFd;
    BinaryParams params;
    bool fixedEntryBreakpoint = false;
    void convertArgs(vector<string> args, char** execArgs);
    bool readBinaryHeader();
    void fixEntryBreakpoint();
    vector<unsigned char> injectPipeModeAssemblyVec;
    vector<unsigned char> preparePipeModeAssemblyInjection();
    string prepareAction(pid_t pid, char* bytes, unsigned int bytesLen);
    unsigned long long int readReg(pid_t pid, Register reg);
    unsigned long long int readMem(pid_t pid, unsigned long long int addr);
    void writeReg(pid_t pid, Register reg, unsigned long long int val);
    void writeMem(pid_t pid, unsigned long long int addr, unsigned long long int val);
    void endBp(pid_t pid);
    bool listenInput(pid_t pid);
public:
    enum DbgCode {
        TARGET_READY = 0x1,
        END_BREAKPOINT = 0x2,
        READ_REG = 0x3,
        READ_MEM = 0x4,
        WRITE_REG = 0x5,
        WRITE_MEM = 0x6,
    };
    HadesDbg(BinaryParams params) : params(params){this->injectPipeModeAssemblyVec = this->preparePipeModeAssemblyInjection();};
    void run();
    void handleExit();
};

#endif