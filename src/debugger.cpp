#include <debugger.h>
#include <sstream>
#include <log.h>
#include <iomanip>
#include <string.h>
#include <thread>
#include <chrono>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <fcntl.h>

using namespace std;

/*
mov rsi, 0x1000
mov rdx, 0x7
mov r10, 0x1
mov r8, 0x66
mov r9, 0x0
syscall
int3
 */
unsigned char mallocAssembly[] = {
        0x48,0xc7,0xc6,0x00,0x10,0x00,0x00,0x48,0xc7,
        0xc2,0x07,0x00,0x00,0x00,0x49,0xc7,0xc2,0x01,
        0x00,0x00,0x00,0x49,0xc7,0xc0,0x66,0x00,0x00,
        0x00,0x49,0xc7,0xc1,0x00,0x00,0x00,0x00,0x0f,
        0x05,0xcc
};

bool HadesDbg::readBinaryHeader() {
    ifstream binaryRead(this->params.binaryPath, ifstream::binary);
    if(binaryRead) {
        binaryRead.seekg(0, binaryRead.end);
        int len = binaryRead.tellg();
        binaryRead.seekg(0, binaryRead.beg);
        if(len >= 0x20) {
            char* idField = (char*) malloc(0x20);
            binaryRead.read(idField, 0x20);
            if(idField[0] == 0x7f && idField[1] == 'E' && idField[2] == 'L' && idField[3] == 'F') {
                if(idField[4] == 2) {
                    if(idField[5] == 1) {
                        if(idField[7] == 0 || idField[7] == 3) {
                            this->replacedFileEntry = (unsigned char*) malloc(sizeof(mallocAssembly));
                            this->fileEntry = *((unsigned long long int*)(idField + 0x18));
                            if(this->fileEntry >= 0x400000) this->fileEntry -= 0x400000;
                            binaryRead.seekg(this->fileEntry, binaryRead.beg);
                            binaryRead.read((char*)this->replacedFileEntry, sizeof(mallocAssembly));
                            binaryRead.close();
                            ofstream binaryWrite;
                            binaryWrite.open(this->params.binaryPath, ofstream::binary | ofstream::out | ofstream::in);
                            binaryWrite.seekp(this->fileEntry, binaryWrite.beg);
                            const char breakpoint = 0xCC;
                            binaryWrite.write(&breakpoint, 1);
                            stringstream msg;
                            msg << "Replaced byte 0x" << hex << setfill('0') << setw(2) << +this->replacedFileEntry[0] << " at address 0x" << hex << setfill('0') << setw(8) << +this->fileEntry << " (entry point) with byte 0xcc (breakpoint).";
                            Logger::getLogger().log(LogLevel::INFO, msg.str());
                            binaryWrite.close();
                            return true;
                        } else Logger::getLogger().log(LogLevel::FATAL, "The specified file can't be run on a Linux architecture.");
                    } else Logger::getLogger().log(LogLevel::FATAL, "HadesDbg currently handles little endian files only.");
                } else Logger::getLogger().log(LogLevel::FATAL, "HadesDbg currently handles x64 files only.");
            } else Logger::getLogger().log(LogLevel::FATAL, "The specified binary is not a ELF file.");
        } else {
            stringstream error;
            error << "The specified binary file is only " << len << " bytes long.";
            Logger::getLogger().log(LogLevel::FATAL, error.str());
        }
        binaryRead.close();
    } else Logger::getLogger().log(LogLevel::FATAL, "An error occured while trying to open the specified binary file.");
    return false;
}

void HadesDbg::convertArgs(vector<string> args, char** execArgs) {
    int argsAmount = 0;
    for (int i = 0; i < args.size(); i++) {
        execArgs[argsAmount++] = strdup(args[i].c_str());
    }
    execArgs[argsAmount++] = NULL;
}

void HadesDbg::fixEntryBreakpoint() {
    this->fixedEntryBreakpoint = true;
    ofstream binaryWrite;
    do {
        binaryWrite.open(this->params.binaryPath, ofstream::binary | ofstream::out | ofstream::in);
        this_thread::sleep_for(std::chrono::milliseconds(10));
    } while(!binaryWrite.is_open());
    binaryWrite.seekp(this->fileEntry, binaryWrite.beg);
    binaryWrite.write((char*)this->replacedFileEntry, 1);
    binaryWrite.close();
    Logger::getLogger().log(LogLevel::INFO, "Restored breakpoint byte.");
}

void HadesDbg::handleExit() {
    kill(this->pid, SIGKILL);
    if(!this->fixedEntryBreakpoint) this->fixEntryBreakpoint();
}

void HadesDbg::run() {
    if(!this->readBinaryHeader()) return;
    this->pid = fork();
    if(!this->pid) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        char* execArgs[1024];
        convertArgs(params.binaryArgs, execArgs);
        execv(this->params.binaryPath.c_str(), execArgs);
    }
    waitpid(this->pid, NULL, 0);
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    //breakpoint set in binary
    waitpid(this->pid, NULL, 0);
    char file[64];
    sprintf(file, "/proc/%ld/mem", (long)this->pid);
    this->memoryFd = open(file, O_RDWR);
    if(this->memoryFd != -1) {
        mallocAssembly[0x18] = (unsigned char) this->memoryFd;
        user_regs_struct regs;
        Logger::getLogger().log(LogLevel::SUCCESS, "Target reached entry breakpoint !");
        Logger::getLogger().log(LogLevel::INFO, "Allocating pipe mode area...");
        ptrace(PTRACE_GETREGS,this->pid,NULL,&regs);
        regs.rip--;
        user_regs_struct savedRegs = regs;
        this->effectiveEntry = regs.rip;
        pwrite(this->memoryFd, &mallocAssembly, sizeof(mallocAssembly), this->effectiveEntry);
        ptrace(PTRACE_SETREGS,this->pid,NULL,&regs);
        ptrace(PTRACE_CONT, pid, NULL, NULL);
        waitpid(this->pid, NULL, 0);
        ptrace(PTRACE_GETREGS,this->pid,NULL,&regs);
        unsigned long long mallocStart = regs.rdi;
        if(mallocStart) Logger::getLogger().log(LogLevel::SUCCESS, "Success !");
        else {
            Logger::getLogger().log(LogLevel::FATAL, "Failure...");
            close(this->memoryFd);
            this->handleExit();
        }
        pwrite(this->memoryFd, this->replacedFileEntry, sizeof(mallocAssembly), this->effectiveEntry);
        ptrace(PTRACE_SETREGS,this->pid,NULL,&savedRegs);
        Logger::getLogger().log(LogLevel::INFO, "Injecting pipe mode in child process...");
        //write assembly in mapped region
        //inject pipe breakpoint at entry point
        if(!ptrace(PTRACE_DETACH,this->pid,NULL,NULL)) {
            Logger::getLogger().log(LogLevel::SUCCESS, "Successfully detached from child process !");
            Logger::getLogger().log(LogLevel::INFO, "Entering pipe mode.");
            while(true);
        } else Logger::getLogger().log(LogLevel::FATAL, "Failed to detach from child process...");
        close(this->memoryFd);
    } else Logger::getLogger().log(LogLevel::FATAL, "Unable to access memory of running target. Missing rights ?");
    if(!this->fixedEntryBreakpoint) this->fixEntryBreakpoint();
}