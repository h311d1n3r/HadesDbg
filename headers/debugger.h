#ifndef HADESDBG_DEBUGGER_H
#define HADESDBG_DEBUGGER_H

#include <main_types.h>
#include <fstream>
#include <sys/user.h>
#include <cstdlib>
#include <utility>
#include <vector>
#include <regs.h>

using namespace std;

#define BigInt unsigned long long int

class HadesDbg {
private:
    BigInt fileEntry, effectiveEntry;
    unsigned char* replacedFileEntry;
    pid_t pid;
    int memoryFd;
    BinaryParams params;
    bool fixedEntryBreakpoint = false;
    static void convertArgs(const vector<string>& args, char** execArgs);
    bool readBinaryHeader();
    void fixEntryBreakpoint();
    vector<unsigned char> injectPipeModeAssemblyVec;
    vector<unsigned char> pipeModeAssemblyVec;
    static vector<unsigned char> preparePipeModeAssemblyInjection(vector<unsigned char> pipeModeAssemblyVec);
    static vector<unsigned char> preparePipeModeAssembly();
    static string prepareAction(pid_t pid, char* bytes, unsigned int bytesLen);
    BigInt readReg(pid_t sonPid, Register reg);
    BigInt readMem(pid_t sonPid, BigInt addr);
    void writeReg(pid_t sonPid, Register reg, BigInt val);
    void writeMem(pid_t sonPid, BigInt addr, BigInt val);
    void endBp(pid_t sonPid);
    bool listenInput(pid_t sonPid);
public:
    enum DbgCode {
        TARGET_READY = 0x1,
        END_BREAKPOINT = 0x2,
        READ_REG = 0x3,
        READ_MEM = 0x4,
        WRITE_REG = 0x5,
        WRITE_MEM = 0x6,
    };
    explicit HadesDbg(BinaryParams params) : params(move(params)){
        this->fileEntry = 0;
        this->effectiveEntry = 0;
        this->replacedFileEntry = nullptr;
        this->pid = 0;
        this->memoryFd = 0;
        this->pipeModeAssemblyVec = preparePipeModeAssembly();
        this->injectPipeModeAssemblyVec = preparePipeModeAssemblyInjection(this->pipeModeAssemblyVec);
    };
    void run();
    void handleExit();
};

#endif