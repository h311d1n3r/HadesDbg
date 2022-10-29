#ifndef HADESDBG_EXPR_INTERPRETER_H
#define HADESDBG_EXPR_INTERPRETER_H

#include <string>
#include <main_types.h>
#include <regs.h>
#include <debugger.h>

using namespace std;

class ExprInterpreter {
private:
    pid_t pid;
    HadesDbg* debugger;
    BigInt entryOff;
public:
    ExprInterpreter(pid_t pid, HadesDbg* debugger, BigInt entryOff) : pid(pid), debugger(debugger), entryOff(entryOff) {};
    BigInt interpret(string mem);
};

#endif //HADESDBG_EXPR_INTERPRETER_H
