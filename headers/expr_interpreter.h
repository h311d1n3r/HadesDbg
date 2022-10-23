#ifndef HADESDBG_EXPR_INTERPRETER_H
#define HADESDBG_EXPR_INTERPRETER_H

#include <string>
#include <main_types.h>
#include <regs.h>

using namespace std;

typedef BigInt (*ReadMemFunc)(pid_t sonPid, BigInt addr);
typedef BigInt (*ReadRegFunc)(pid_t sonPid, Register reg);

class ExprInterpreter {
private:
    pid_t pid;
    ReadMemFunc readMem;
    ReadRegFunc readReg;
    BigInt entryOff;
public:
    ExprInterpreter(pid_t pid, ReadMemFunc readMem, ReadRegFunc readReg, BigInt entryOff) : pid(pid), readMem(readMem), readReg(readReg), entryOff(entryOff) {};
    BigInt interpret(string mem);
};

#endif //HADESDBG_EXPR_INTERPRETER_H
