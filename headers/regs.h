#ifndef HADESDBG_REGS_H
#define HADESDBG_REGS_H

#include <string>
#include <map>

using namespace std;

enum Register {
    RSP = 0x0,
    RAX = 0x8,
    RBX = 0x10,
    RCX = 0x18,
    RDX = 0x20,
    RDI = 0x28,
    RSI = 0x30,
    R8 = 0x38,
    R9 = 0x40,
    R10 = 0x48,
    R11 = 0x50,
    R12 = 0x58,
    R13 = 0x60,
    R14 = 0x68,
    R15 = 0x70,
    RBP = 0x78,
    RIP = 0x80,
};

extern map<string, Register> registerFromName;

#endif //HADESDBG_REGS_H
