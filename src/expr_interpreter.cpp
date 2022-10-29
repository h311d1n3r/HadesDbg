#include <expr_interpreter.h>
#include <utils.h>

BigInt ExprInterpreter::interpret(std::string mem) {
    mem = trim(mem);
    if(!mem.length()) return 0;
    if(!mem.find_first_of("[")) {
        int lastIndex = mem.find_last_of("]");
        string pointerStr = mem.substr(1, lastIndex-1);
        BigInt addr = this->interpret(pointerStr);
        return this->debugger->readMem(pid, addr) + this->interpret(mem.substr(lastIndex+1));
    } else {
        string skipped = "";
        while (mem.length() > 0) {
            if (!mem.find("+")) {
                string addStr = mem.substr(1);
                BigInt val1 = this->interpret(skipped);
                BigInt val2 = this->interpret(addStr);
                BigInt result = val1 + val2;
                return result;
            } else if (!mem.find("-")) {
                string subStr = mem.substr(1);
                BigInt val1 = this->interpret(skipped);
                BigInt val2 = this->interpret(subStr);
                BigInt result = val1 - val2;
                return result;
            } else if (!mem.find("*")) {
                string mulStr = mem.substr(1);
                BigInt val1 = this->interpret(skipped);
                BigInt val2 = this->interpret(mulStr);
                BigInt result = val1 * val2;
                return result;
            } else if (!mem.find("/")) {
                string divStr = mem.substr(1);
                BigInt val1 = this->interpret(skipped);
                BigInt val2 = this->interpret(divStr);
                BigInt result = val1 / val2;
                return result;
            } else {
                skipped += mem.at(0);
                mem = mem.substr(1);
            }
        }
        if (skipped.length() >= 0) {
            BigInt val;
            bool relativeToEntry = false;
            if (!skipped.find("@")) {
                relativeToEntry = true;
                skipped = skipped.substr(1);
            }
            if (!inputToNumber(skipped, val)) {
                Register reg = registerFromName[toUppercase(skipped)];
                val = this->debugger->readReg(this->pid, reg);
#if __x86_64__
                if(reg == Register::RIP) val += this->entryOff;
#else
                if(reg == Register::EIP) val += this->entryOff;
#endif
            }
            if (relativeToEntry) val = val + this->entryOff;
            return val;
        }
    }
    return 0;
}