#include <sys/ptrace.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/user.h>

int main() {
    pid_t pid = fork();
    if(!pid) {
        //son
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        unsigned long long int test = 0x1234;
        unsigned long long int test2 = 15;
        __asm__("int $3");
        test2+=test;
        test2-=test;
        test2+=test;
        test2-=test;
    } else {
        //father
        waitpid(pid, nullptr, 0);
        user_regs_struct regs = {};
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
        unsigned long long int testAddr = regs.rbp-0xf0;
        unsigned long long int testVal = ptrace(PTRACE_PEEKDATA, pid, testAddr, 0);
        testVal+=0x1111;
        ptrace(PTRACE_POKEDATA, pid, testAddr, testVal);
        ptrace(PTRACE_CONT, pid, 0, 0);
    }
    return 0;
}
