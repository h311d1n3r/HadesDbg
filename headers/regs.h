#ifndef HADESDBG_REGS_H
#define HADESDBG_REGS_H

#include <string>
#include <map>
#include <vector>

using namespace std;

#if __x86_64__
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
    RIP = 0x80
};
#else
enum Register {
    ESP = 0x0,
    EAX = 0x4,
    EBX = 0x8,
    ECX = 0xc,
    EDX = 0x10,
    EDI = 0x14,
    ESI = 0x18,
    EBP = 0x1c,
    EIP = 0x20
};
#endif

extern map<string, Register> registerFromName;

extern vector<string> orderedRegsNames;

#endif //HADESDBG_REGS_H
