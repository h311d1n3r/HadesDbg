#ifndef HADESDBG_DEBUGGER_H
#define HADESDBG_DEBUGGER_H

#include <main_types.h>
#include <fstream>
#include <sys/user.h>
#include <cstdlib>
#include <vector>

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
public:
    HadesDbg(BinaryParams params) : params(params){};
    void run();
    void handleExit();
};

#endif