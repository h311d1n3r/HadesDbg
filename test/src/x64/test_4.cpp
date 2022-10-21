#include <sys/ptrace.h>

int main() {
    long res = ptrace(PTRACE_TRACEME, 0, 0, 0);
    int i=0;
    i+=1;
    i-=3;
    i*=2;
    return 0;
}
