#include <debugger.h>
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

unsigned char pipeModeAssembly[] = {
    //restore rax
    0x58, 0x58,

    //push registers
    0x50,0x53,0x51,0x52,0x57,0x56,0x41,0x50,0x41,
    0x51,0x41,0x52,0x41,0x53,0x41,0x54,0x41,0x55,
    0x41,0x56,0x41,0x57,0x55,

    //allocate space for pipe file name (./pipe_XXXXXXXX.hades0).
    //start of allocated region is moved to rbx
    0x48,0xc7,0xc0,0x09,0x00,0x00,0x00,0x48,0xc7,
    0xc7,0x00,0x00,0x00,0x00,0x48,0xc7,0xc6,0x16,
    0x00,0x00,0x00,0x48,0xc7,0xc2,0x07,0x00,0x00,
    0x00,0x49,0xc7,0xc2,0x22,0x00,0x00,0x00,0x49,
    0xc7,0xc0,0xff,0xff,0xff,0xff,0x49,0xc7,0xc1,
    0x00,0x00,0x00,0x00,0x0f,0x05,0x48,0x89,0xc3,

    //get pid
    0x48,0xc7,0xc0,0x27,0x00,0x00,0x00,0x0f,0x05,

    //write pipe file name in allocated region
    //start of allocated region is moved to rdi
    0x48,0x89,0xdf,0x48,0xc7,0x03,0x2e,0x2f,0x70,
    0x69,0x48,0x83,0xc3,0x04,0xc6,0x03,0x70,0x48,
    0xff,0xc3,0xc6,0x03,0x65,0x48,0xff,0xc3,0xc6,
    0x03,0x5f,0x48,0x89,0xda,0x48,0x83,0xc3,0x08,
    0x48,0x89,0xc1,0x48,0x83,0xe1,0x0f,0x48,0x83,
    0xf9,0x09,0x77,0x06,0x48,0x83,0xc1,0x30,0xeb,
    0x04,0x48,0x83,0xc1,0x57,0x88,0x0b,0x48,0xff,
    0xcb,0x48,0xc1,0xe8,0x04,0x48,0x39,0xd3,0x75,
    0xdb,0x48,0x83,0xc3,0x09,0x48,0xc7,0x03,0x2e,
    0x68,0x61,0x64,0x48,0x83,0xc3,0x04,0xc6,0x03,
    0x65,0x48,0xff,0xc3,0xc6,0x03,0x73,0x48,0xff,
    0xc3,0xc6,0x03,0x00,

    //open pipe file in create mode
    0x48,0xc7,0xc0,0x02,0x00,0x00,0x00,0x48,0xc7,
    0xc6,0x42,0x00,0x00,0x00,0x48,0xc7,0xc2,0xff,
    0x01,0x00,0x00,0x0f,0x05,

    //prints 'aaa' from the son
    /*0x48,0xc7,0xc0,0x01,0x00,0x00,0x00,0x48,0xc7,
    0xc7,0x01,0x00,0x00,0x00,0x68,0x61,0x61,0x61,
    0x00,0x48,0x89,0xe6,0x48,0xc7,0xc2,0x03,0x00,
    0x00,0x00,0x0f,0x05,0x48,0x83,0xc4,0x08,*/

    //pop registers
    0x5d,0x41,0x5f,0x41,0x5e,0x41,0x5d,0x41,0x5c,
    0x41,0x5b,0x41,0x5a,0x41,0x59,0x41,0x58,0x5e,
    0x5f,0x5a,0x59,0x5b,0x58,

    //replaced instructions
    0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
    0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
    0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
    0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
    0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
    0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
    0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
    0x90,

    //return
    0x48,0x83,0xec,0x08,0x50,0x48,0x83,0xc4,0x10,
    0x48,0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x50,0x48,0x83,0xec,0x08,0x58,0xc3
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
                            this->replacedFileEntry = (unsigned char*) malloc(this->injectPipeModeAssemblyVec.size());
                            this->fileEntry = *((unsigned long long int*)(idField + 0x18));
                            if(this->fileEntry >= 0x400000) this->fileEntry -= 0x400000;
                            binaryRead.seekg(this->fileEntry, binaryRead.beg);
                            binaryRead.read((char*)this->replacedFileEntry, this->injectPipeModeAssemblyVec.size());
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

vector<unsigned char> HadesDbg::preparePipeModeAssemblyInjection() {
    /*
    mov rax, 0x9
    mov rdi, 0x0
    mov rsi, 0x0
    mov rdx, 0x7
    mov r10, 0x22
    mov r8, -0x1
    mov r9, 0x0
    syscall
    */
    unsigned char allocArr[] = {
        0x48,0xc7,0xc0,0x09,0x00,0x00,0x00,0x48,0xc7,
        0xc7,0x00,0x00,0x00,0x00,0x48,0xc7,0xc6,0x00,
        0x00,0x00,0x00,0x48,0xc7,0xc2,0x07,0x00,0x00,
        0x00,0x49,0xc7,0xc2,0x22,0x00,0x00,0x00,0x49,
        0xc7,0xc0,0xff,0xff,0xff,0xff,0x49,0xc7,0xc1,
        0x00,0x00,0x00,0x00,0x0f,0x05
    };
    vector<unsigned char> vec;
    vec.insert(vec.end(), allocArr, allocArr + sizeof(allocArr));
    vec.push_back(0x50);
    unsigned char movArr[] = {0x48,0xc7,0x00};
    unsigned char addArr[] = {0x48,0x83,0xc0,0x04};
    for(int i = 0; i < sizeof(pipeModeAssembly); i+=4) {
        vec.insert(vec.end(), movArr, movArr + sizeof(movArr));
        for(int i2 = i; i2 < i + 4; i2++) {
            if(i2 < sizeof(pipeModeAssembly)) vec.push_back(pipeModeAssembly[i2]);
            else vec.push_back(0x0);
        }
        if(i + 4 < sizeof(pipeModeAssembly)) {
            vec.insert(vec.end(), addArr, addArr + sizeof(addArr));
        }
    }
    vec.push_back(0x58);
    vec.push_back(0xcc);
    return vec;
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
        unsigned char breakpointCall[] = {
            0x50,0x48,0xb8,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0xff,0xd0
        };
        user_regs_struct regs;
        Logger::getLogger().log(LogLevel::SUCCESS, "Target reached entry breakpoint !");
        Logger::getLogger().log(LogLevel::INFO, "Injecting pipe mode in child process...");
        ptrace(PTRACE_GETREGS,this->pid,NULL,&regs);
        regs.rip--;
        user_regs_struct savedRegs = regs;
        this->effectiveEntry = regs.rip;
        pwrite(this->memoryFd, this->replacedFileEntry, 1, this->effectiveEntry);
        unsigned int injectPipeModeAssemblySize = this->injectPipeModeAssemblyVec.size();
        map<unsigned long long int, unsigned long long int> allocAddrFromBpAddr;
        unsigned int pipeModeAssemblySize = sizeof(pipeModeAssembly);
        map<unsigned long long int, unsigned char>::iterator breakpoint;
        for(breakpoint = this->params.breakpoints.begin(); breakpoint != this->params.breakpoints.end(); breakpoint++) {
            unsigned long long int breakpointAddr = breakpoint->first - this->params.entryAddress + this->effectiveEntry;
            unsigned char breakpointLength = breakpoint->second;
            pread(this->memoryFd, pipeModeAssembly + pipeModeAssemblySize - 64 - 26, breakpointLength, breakpointAddr);
            unsigned long long int returnAddr = breakpointAddr + breakpointLength;
            memcpy(pipeModeAssembly + pipeModeAssemblySize - 0xf, &returnAddr, sizeof(returnAddr));
            this->injectPipeModeAssemblyVec = this->preparePipeModeAssemblyInjection();
            unsigned char *injectPipeModeAssembly = &(this->injectPipeModeAssemblyVec)[0];
            memcpy(injectPipeModeAssembly + 0x11, &pipeModeAssemblySize, sizeof(pipeModeAssemblySize));
            pwrite(this->memoryFd, injectPipeModeAssembly, injectPipeModeAssemblySize, this->effectiveEntry);
            memset(pipeModeAssembly + pipeModeAssemblySize - 64 - 26, 0x90, 64);
            ptrace(PTRACE_SETREGS, this->pid, NULL, &savedRegs);
            ptrace(PTRACE_CONT, pid, NULL, NULL);
            waitpid(this->pid, NULL, 0);
            ptrace(PTRACE_GETREGS,this->pid,NULL,&regs);
            unsigned long long allocStart = regs.rax;
            if(!allocStart) {
                Logger::getLogger().log(LogLevel::FATAL, "Failure...");
                close(this->memoryFd);
                this->handleExit();
            }
            allocAddrFromBpAddr[breakpointAddr] = allocStart;
        }
        Logger::getLogger().log(LogLevel::SUCCESS, "Success !");
        ptrace(PTRACE_SETREGS, this->pid, NULL, &savedRegs);
        pwrite(this->memoryFd, this->replacedFileEntry, injectPipeModeAssemblySize, this->effectiveEntry);
        map<unsigned long long int, unsigned long long int>::iterator injectData;
        stringstream injectBpMsg;
        injectBpMsg << "Injecting \033[;37m" << allocAddrFromBpAddr.size() << "\033[;36m breakpoint(s)...";
        Logger::getLogger().log(LogLevel::INFO, injectBpMsg.str());
        for(injectData = allocAddrFromBpAddr.begin(); injectData != allocAddrFromBpAddr.end(); injectData++) {
            unsigned long long int breakpointAddr = injectData->first;
            unsigned long long int allocStart = injectData->second;
            memcpy(breakpointCall + 0x3, &allocStart, sizeof(allocStart));
            /*stringstream test;
            test << hex << +allocStart;
            Logger::getLogger().log(LogLevel::FATAL, test.str());*/
            pwrite(this->memoryFd, &breakpointCall, sizeof(breakpointCall), breakpointAddr);
        }
        Logger::getLogger().log(LogLevel::SUCCESS, "Done !");
        if(!ptrace(PTRACE_DETACH,this->pid,NULL,NULL)) {
            Logger::getLogger().log(LogLevel::SUCCESS, "Successfully detached from child process !");
            Logger::getLogger().log(LogLevel::INFO, "Entering pipe mode.");
            while(true);
        } else Logger::getLogger().log(LogLevel::FATAL, "Failed to detach from child process...");
        close(this->memoryFd);
    } else Logger::getLogger().log(LogLevel::FATAL, "Unable to access memory of running target. Missing rights ?");
    if(!this->fixedEntryBreakpoint) this->fixEntryBreakpoint();
}