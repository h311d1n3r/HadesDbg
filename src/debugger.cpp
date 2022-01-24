#include <debugger.h>
#include <log.h>
#include <iomanip>
#include <string.h>
#include <thread>
#include <chrono>
#include <sys/wait.h>
#include <unistd.h>
#include <filesystem>
#include <utils.h>
#include <sys/ptrace.h>
#include <fcntl.h>
#include <iostream>

using Code = HadesDbg::DbgCode;
using namespace std;

unsigned char pipeModeAssembly[] = {
    //restore rax
    0x58, 0x58,

    //push registers
    0x54,0x50,0x53,0x51,0x52,0x57,0x56,0x41,0x50,
    0x41,0x51,0x41,0x52,0x41,0x53,0x41,0x54,0x41,
    0x55,0x41,0x56,0x41,0x57,0x55,

    //push rip
    0x48,0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x50,

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
    //start of allocated region is pushed on the stack
    //file descriptor is in rax
    0x48,0xc7,0xc0,0x02,0x00,0x00,0x00,0x48,0xc7,
    0xc6,0x42,0x00,0x00,0x00,0x48,0xc7,0xc2,0xff,
    0x01,0x00,0x00,0x0f,0x05,0x57,

    //allocate region for read and write operations
    //file descriptor is moved to rbx
    //start of allocation is in rax
    0x48,0x89,0xc3,0x48,0xc7,0xc0,0x09,0x00,0x00,
    0x00,0x48,0xc7,0xc7,0x00,0x00,0x00,0x00,0x48,
    0xc7,0xc6,0xff,0x00,0x00,0x00,0x48,0xc7,0xc2,
    0x07,0x00,0x00,0x00,0x49,0xc7,0xc2,0x22,0x00,
    0x00,0x00,0x49,0xc7,0xc0,0xff,0xff,0xff,0xff,
    0x49,0xc7,0xc1,0x00,0x00,0x00,0x00,0x0f,0x05,

    //write target ready message to file
    //start of allocation is moved to rsi
    0x48,0x89,0xc6,0xc6,0x00,Code::TARGET_READY,0x48,0x89,0xdf,
    0x48,0xc7,0xc0,0x01,0x00,0x00,0x00,0x48,0xc7,
    0xc2,0x01,0x00,0x00,0x00,0x0f,0x05,

    //close file
    0x48,0xc7,0xc0,0x03,0x00,0x00,0x00,0x0f,0x05,

    //wait for 1 second
    //start of allocation is put in r8
    0x48,0x89,0xf2,0x48,0xc7,0x02,0x01,0x00,0x00,
    0x00,0x48,0x83,0xc2,0x04,0x48,0xc7,0x02,0x00,
    0x00,0x00,0x00,0x48,0x83,0xc2,0x04,0x48,0xc7,
    0x02,0x00,0x00,0x00,0x00,0x48,0x83,0xc2,0x04,
    0x48,0xc7,0x02,0x00,0x00,0x00,0x00,0x48,0x89,
    0xf7,0x49,0x89,0xf8,0x48,0x83,0xc6,0x08,0x48,
    0xc7,0xc0,0x23,0x00,0x00,0x00,0x0f,0x05,

    //open pipe file again
    //if it fails, jump back to wait
    0x5f,0x57,0x48,0xc7,0xc0,0x02,0x00,0x00,0x00,
    0x48,0xc7,0xc6,0x02,0x00,0x00,0x00,0x48,0xc7,
    0xc2,0xff,0x01,0x00,0x00,0x0f,0x05,0x4c,0x89,
    0xc6,0x48,0x83,0xf8,0x00,0x78,0xa0,

    //read in allocated region
    //if target ready message, jump back
    //file descriptor is in rbx
    0x48,0x89,0xc7,0x4c,0x89,0xc6,0x48,0xc7,0xc2,
    0xff,0x00,0x00,0x00,0x48,0xc7,0xc0,0x00,0x00,
    0x00,0x00,0x0f,0x05,0x41,0x80,0x38,Code::TARGET_READY,0x74,
    0x84,0x48,0x89,0xfb,

    //read register
    0x41,0x80,0x38,Code::READ_REG,0x75,0x4c,0x48,0x83,0xc4,
    0x08,0x49,0x03,0x60,0x01,0x41,0x59,0x49,0x2b,
    0x60,0x01,0x48,0x83,0xec,0x10,0x41,0xc6,0x00,
    Code::TARGET_READY,0x4d,0x89,0x48,0x01,0x41,0xc6,0x40,0x09,
    0x00,0x48,0x89,0xdf,0x48,0xc7,0xc6,0x00,0x00,
    0x00,0x00,0x48,0xc7,0xc2,0x00,0x00,0x00,0x00,
    0x48,0xc7,0xc0,0x08,0x00,0x00,0x00,0x0f,0x05,
    0x4c,0x89,0xc6,0x48,0xc7,0xc2,0x0a,0x00,0x00,
    0x00,0x48,0xc7,0xc0,0x01,0x00,0x00,0x00,0x0f,
    0x05,

    //read memory value
    0x41,0x80,0x38,Code::READ_MEM,0x75,0x41,0x4d,0x8b,0x48,
    0x01,0x4d,0x8b,0x09,0x41,0xc6,0x00,Code::TARGET_READY,0x4d,
    0x89,0x48,0x01,0x41,0xc6,0x40,0x09,0x00,0x48,
    0x89,0xdf,0x48,0xc7,0xc6,0x00,0x00,0x00,0x00,
    0x48,0xc7,0xc2,0x00,0x00,0x00,0x00,0x48,0xc7,
    0xc0,0x08,0x00,0x00,0x00,0x0f,0x05,0x4c,0x89,
    0xc6,0x48,0xc7,0xc2,0x0a,0x00,0x00,0x00,0x48,
    0xc7,0xc0,0x01,0x00,0x00,0x00,0x0f,0x05,

    //write register
    0x41,0x80,0x38,Code::WRITE_REG,0x75,0x4a,0x48,0x83,0xc4,
    0x10,0x49,0x03,0x60,0x01,0x41,0xff,0x70,0x09,
    0x48,0x83,0xec,0x08,0x49,0x2b,0x60,0x01,0x41,
    0xc6,0x00,Code::TARGET_READY,0x41,0xc6,0x40,0x01,0x00,0x48,
    0x89,0xdf,0x48,0xc7,0xc6,0x00,0x00,0x00,0x00,
    0x48,0xc7,0xc2,0x00,0x00,0x00,0x00,0x48,0xc7,
    0xc0,0x08,0x00,0x00,0x00,0x0f,0x05,0x4c,0x89,
    0xc6,0x48,0xc7,0xc2,0x02,0x00,0x00,0x00,0x48,
    0xc7,0xc0,0x01,0x00,0x00,0x00,0x0f,0x05,

    //write memory value
    0x41,0x80,0x38,Code::WRITE_MEM,0x75,0x41,0x4d,0x8b,0x48,
    0x01,0x4d,0x8b,0x50,0x09,0x4d,0x89,0x11,0x41,
    0xc6,0x00,Code::TARGET_READY,0x41,0xc6,0x40,0x01,0x00,0x48,
    0x89,0xdf,0x48,0xc7,0xc6,0x00,0x00,0x00,0x00,
    0x48,0xc7,0xc2,0x00,0x00,0x00,0x00,0x48,0xc7,
    0xc0,0x08,0x00,0x00,0x00,0x0f,0x05,0x4c,0x89,
    0xc6,0x48,0xc7,0xc2,0x02,0x00,0x00,0x00,0x48,
    0xc7,0xc0,0x01,0x00,0x00,0x00,0x0f,0x05,

    //close pipe file
    0x48,0xc7,0xc0,0x03,0x00,0x00,0x00,0x48,0x89,
    0xdf,0x0f,0x05,

    //check end_breakpoint
    0x41,0x80,0x38,Code::END_BREAKPOINT,0x0f,0x85,0x3b,0xfe,0xff,
    0xff,

    //delete pipe file
    //pipe file name is in rdi
    0x5f,0x48,0xc7,0xc0,0x57,0x00,0x00,0x00,0x0f,
    0x05,

    //free read/write region
    0x48,0xc7,0xc0,0x0b,0x00,0x00,0x00,0x4c,
    0x89,0xc7,0x48,0xc7,0xc6,0xff,0x00,0x00,
    0x00,0x0f,0x05,

    //free pipe file name region
    0x48,0xc7,0xc0,0x0b,0x00,0x00,0x00,0x48,
    0xc7,0xc6,0x16,0x00,0x00,0x00,0x0f,0x05,

    //prevent pop of rip
    0x48,0x83,0xc4,0x8,

    //pop registers
    0x5d,0x41,0x5f,0x41,0x5e,0x41,0x5d,0x41,0x5c,
    0x41,0x5b,0x41,0x5a,0x41,0x59,0x41,0x58,0x5e,
    0x5f,0x5a,0x59,0x5b,0x58,

    //prevent pop of rsp
    0x48,0x83,0xc4,0x8,

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
    for(const filesystem::directory_entry &entry : filesystem::directory_iterator("./")) {
        const string path = entry.path().u8string();
        if (endsWith(toUppercase(path), ".HADES")) {
            unsigned long long int pid = 0;
            if (!inputToNumber("0x" + path.substr(path.length() - 14, 8), pid)) {
                Logger::getLogger().log(LogLevel::ERROR, "Unable to read breakpoint PID !");
            }
            stringstream killStr;
            killStr << "Killing son with PID: \033[;37m"  << hex << +pid << "\033[;36m";
            Logger::getLogger().log(LogLevel::INFO, killStr.str());
            remove(path.c_str());
            kill(pid, SIGKILL);
        }
    }
    stringstream lsofCommand;
    lsofCommand << "lsof \"" << this->params.binaryPath << "\"";
    string lsofResult = executeCommand(lsofCommand.str().c_str());
    vector<string> lsofResLines;
    split(lsofResult, '\n', lsofResLines);
    for(int i = 1; i < lsofResLines.size(); i++) {
        string line = lsofResLines[i];
        if(line.length() > 0) {
            vector<string> lineFields;
            split(line, ' ', lineFields);
            if(lineFields.size() >= 2) {
                string pidStr = lineFields[1];
                unsigned long long int pid = 0;
                if(inputToNumber(pidStr, pid)) {
                    stringstream killStr;
                    killStr << "Killing son with PID: \033[;37m"  << hex << +pid << "\033[;36m";
                    Logger::getLogger().log(LogLevel::INFO, killStr.str());
                    kill(pid, SIGKILL);
                }
            }
        }
    }
    kill(this->pid, SIGKILL);
    if(!this->fixedEntryBreakpoint) this->fixEntryBreakpoint();
}

string HadesDbg::prepareAction(pid_t pid, char* bytes, unsigned int bytesLen) {
    stringstream filePath;
    filePath << "./pipe_" << setfill('0') << std::setw(8) << hex << pid;
    filePath << ".hades";
    ofstream fileWriter;
    fileWriter.open(filePath.str(), ios::binary | ios::out);
    fileWriter.write(bytes, bytesLen);
    fileWriter.close();
    return filePath.str();
}

unsigned long long int HadesDbg::readReg(pid_t pid, Register reg) {
    const char readRegBytes[] = {
        Code::READ_REG,
        (char)(0x80 - reg), 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    };
    string filePath = this->prepareAction(pid, (char*)readRegBytes, sizeof(readRegBytes));
    char* buffer = (char*) malloc(0xa);
    while(true) {
        ifstream fileReader;
        fileReader.open(filePath, ios::binary | ios::in);
        fileReader.read(buffer, 0xa);
        fileReader.close();
        if(buffer[0] == Code::TARGET_READY) break;
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    unsigned long long int ret = 0;
    memcpy(&ret, buffer+0x1, sizeof(ret));
    free(buffer);
    if(reg == Register::RIP) ret = ret - this->effectiveEntry + this->params.entryAddress;
    return ret;
}

unsigned long long int HadesDbg::readMem(pid_t pid, unsigned long long int addr) {
    char readMemBytes[] = {
        Code::READ_MEM,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    };
    memcpy(readMemBytes + 1, &addr, sizeof(addr));
    string filePath = this->prepareAction(pid, (char*)readMemBytes, sizeof(readMemBytes));
    char* buffer = (char*) malloc(0xa);
    while(true) {
        ifstream fileReader;
        fileReader.open(filePath, ios::binary | ios::in);
        fileReader.read(buffer, 0xa);
        fileReader.close();
        if(buffer[0] == Code::TARGET_READY) break;
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    unsigned long long int ret = 0;
    memcpy(&ret, buffer+0x1, sizeof(ret));
    free(buffer);
    return ret;
}

void HadesDbg::writeReg(pid_t pid, Register reg, unsigned long long int val) {
    char writeRegBytes[] = {
        Code::WRITE_REG,
        (char)(0x80 - reg), 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    };
    memcpy(writeRegBytes + 9, &val, sizeof(val));
    string filePath = this->prepareAction(pid, (char*)writeRegBytes, sizeof(writeRegBytes));
    char* buffer = (char*) malloc(0x1);
    while(true) {
        ifstream fileReader;
        fileReader.open(filePath, ios::binary | ios::in);
        fileReader.read(buffer, 0x1);
        fileReader.close();
        if(buffer[0] == Code::TARGET_READY) break;
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    free(buffer);
}

void HadesDbg::writeMem(pid_t pid, unsigned long long int addr, unsigned long long int val) {
    char writeMemBytes[] = {
            Code::WRITE_MEM,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    };
    memcpy(writeMemBytes + 1, &addr, sizeof(addr));
    memcpy(writeMemBytes + 9, &val, sizeof(val));
    string filePath = this->prepareAction(pid, (char*)writeMemBytes, sizeof(writeMemBytes));
    char* buffer = (char*) malloc(0x1);
    while(true) {
        ifstream fileReader;
        fileReader.open(filePath, ios::binary | ios::in);
        fileReader.read(buffer, 0x1);
        fileReader.close();
        if(buffer[0] == Code::TARGET_READY) break;
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    free(buffer);
}

void HadesDbg::endBp(pid_t pid) {
    const char endBpBytes[] = {
        Code::END_BREAKPOINT
    };
    string filePath = this->prepareAction(pid, (char*)endBpBytes, sizeof(endBpBytes));
    while(filesystem::exists(filePath)) {
        this_thread::sleep_for(chrono::milliseconds(500));
    }
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

bool HadesDbg::listenInput(pid_t pid) {
    string input;
    while(true) {
        getline(cin, input);
        vector<string> params;
        split(input, ' ', params);
        if(params.size() > 0) {
            if(!params[0].compare("exit") || !params[0].compare("e")) return true;
            else if(!params[0].compare("run") || !params[0].compare("r")) {
                stringstream runAskStr;
                runAskStr << "Asking \033[;37m" << hex << pid << "\033[;36m to resume execution...";
                Logger::getLogger().log(LogLevel::INFO, runAskStr.str());
                this->endBp(pid);
                Logger::getLogger().log(LogLevel::SUCCESS, "Success !");
                return false;
            }
            else if(!params[0].compare("readreg") || !params[0].compare("rr")) {
                if(params.size() > 1) {
                    string regName = toUppercase(params[1]);
                    if (registerFromName.count(regName)) {
                        stringstream regAskStr;
                        regAskStr << "Asking \033[;37m" << hex << pid << "\033[;36m for \033[;37m" << regName << " \033[;36mvalue...";
                        Register reg = registerFromName[regName];
                        Logger::getLogger().log(LogLevel::INFO, regAskStr.str());
                        unsigned long long int regValue = readReg(pid, reg);
                        stringstream regValStr;
                        regValStr << regName << ": \033[;37m" << hex << regValue << "\033[;32m";
                        Logger::getLogger().log(LogLevel::SUCCESS, regValStr.str());
                    } else Logger::getLogger().log(LogLevel::WARNING, "First parameter must be a valid register ! \033[;37mreadreg register\033[;33m");
                } else Logger::getLogger().log(LogLevel::WARNING, "This command requires more parameters ! \033[;37mreadreg register\033[;33m");
            }
            else if(!params[0].compare("readmem") || !params[0].compare("rm")) {
                if(params.size() > 2) {
                    string addrStr = params[1];
                    bool entryRelative = false;
                    if(!addrStr.find("@")) {
                        entryRelative = true;
                        addrStr = addrStr.substr(1);
                    }
                    unsigned long long int addr = 0;
                    if(inputToNumber(addrStr, addr)) {
                        string lenStr = params[2];
                        unsigned long long int len = 0;
                        if(inputToNumber(lenStr, len)) {
                            if(len > 0) {
                                stringstream memAskStr;
                                memAskStr << "Asking \033[;37m" << hex << pid << "\033[;36m for \033[;37m" << hex << len
                                          << " \033[;36mbytes at address \033[;37m" << hex << addr << "\033[;36m...";
                                Logger::getLogger().log(LogLevel::INFO, memAskStr.str());
                                stringstream memValStr;
                                memValStr << hex << addr << ":";
                                Logger::getLogger().log(LogLevel::SUCCESS, memValStr.str());
                                for(int i = 0; i < len; i+=8) {
                                    unsigned long long int memValue = readMem(pid, entryRelative ? (addr + i - this->params.entryAddress + this->effectiveEntry) : addr + i);
                                    for(int i2 = 0; i2 < sizeof(memValue) && i+i2 < len; i2++) {
                                        cout << hex << setfill('0') << setw(2) << +(memValue >> (i2 * 8) & 0xff);
                                        if(i2+1 < sizeof(memValue) && i+i2+1 < len) cout << " ";
                                    }
                                    cout << endl;
                                }
                                Logger::getLogger().log(LogLevel::SUCCESS, "Done !");
                            } else Logger::getLogger().log(LogLevel::WARNING, "Length must be a strictly positive value ! \033[;37mreadmem address length\033[;33m");
                        } else Logger::getLogger().log(LogLevel::WARNING, "Second parameter must be a valid length ! \033[;37mreadmem address length\033[;33m");
                    } else Logger::getLogger().log(LogLevel::WARNING, "First parameter must be a valid address ! \033[;37mreadmem address length\033[;33m");
                } else Logger::getLogger().log(LogLevel::WARNING, "This command requires more parameters ! \033[;37mreadmem address length\033[;33m");
            }
            else if(!params[0].compare("writereg") || !params[0].compare("wr")) {
                if(params.size() > 2) {
                    string regName = toUppercase(params[1]);
                    if (registerFromName.count(regName)) {
                        Register reg = registerFromName[regName];
                        string valStr = params[2];
                        unsigned long long int val;
                        if(inputToNumber(valStr, val)) {
                            stringstream regEditStr;
                            regEditStr << "Asking \033[;37m" << hex << pid << "\033[;36m to set \033[;37m" << regName
                                       << "\033[;36m to \033[;37m" << hex << val << "\033[;36m...";
                            Logger::getLogger().log(LogLevel::INFO, regEditStr.str());
                            this->writeReg(pid, reg, val);
                            Logger::getLogger().log(LogLevel::SUCCESS, "Success !");
                        } else Logger::getLogger().log(LogLevel::WARNING, "Second parameter must be a number ! \033[;37mwritereg register value\033[;33m");
                    } else Logger::getLogger().log(LogLevel::WARNING, "First parameter must be a valid register ! \033[;37mwritereg register value\033[;33m");
                } else Logger::getLogger().log(LogLevel::WARNING, "This command requires more parameters ! \033[;37mwritereg register value\033[;33m");
            }
            else if(!params[0].compare("writemem") || !params[0].compare("wm")) {
                if(params.size() > 2) {
                    string addrStr = params[1];
                    bool entryRelative = false;
                    if(!addrStr.find("@")) {
                        entryRelative = true;
                        addrStr = addrStr.substr(1);
                    }
                    unsigned long long int addr;
                    if(inputToNumber(addrStr, addr)) {
                        string hexChain = params[2];
                        stringstream memEditStr;
                        memEditStr << "Asking \033[;37m" << hex << pid << "\033[;36m to write \033[;37m" << hexChain
                                   << "\033[;36m at \033[;37m" << hex << addr << "\033[;36m...";
                        Logger::getLogger().log(LogLevel::INFO, memEditStr.str());
                        for(int i = 0; i < hexChain.length(); i += 2 * sizeof(unsigned long long int)) {
                            unsigned long long int val = 0;
                            int substrLen = i + 2 * sizeof(unsigned long long int) > hexChain.length() ? hexChain.length() - i : 2 * sizeof(unsigned long long int);
                            if(inputToNumber("0x"+hexChain.substr(i, substrLen), val)) {
                                val = invertEndian(val);
                                if(substrLen < 2 * sizeof(unsigned long long int)) {
                                    unsigned long long int oldVal = readMem(pid, entryRelative ? (addr + (i/2) - this->params.entryAddress + this->effectiveEntry) : addr + (i/2));
                                    val >>= sizeof(unsigned long long int) * 8 - (substrLen * 8 / 2);
                                    val += (oldVal >> (substrLen * 8 / 2)) << (substrLen * 8 / 2);
                                }
                                this->writeMem(pid, entryRelative ? (addr + (i/2) - this->params.entryAddress + this->effectiveEntry) : addr + (i/2), val);
                            } else Logger::getLogger().log(LogLevel::WARNING, "Second parameter must be a valid hexadecimal chain ! \033[;37mwritemem address hex_chain\033[;33m");
                        }
                        Logger::getLogger().log(LogLevel::SUCCESS, "Success !");
                    } else Logger::getLogger().log(LogLevel::WARNING, "First parameter must be a valid address ! \033[;37mwritemem address hex_chain\033[;33m");
                } else Logger::getLogger().log(LogLevel::WARNING, "This command requires more parameters ! \033[;37mwritemem address hex_chain\033[;33m");
            }
            else Logger::getLogger().log(LogLevel::WARNING, "Unknown command !");
        } else Logger::getLogger().log(LogLevel::WARNING, "Please input a command !");
    }
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
            memcpy(pipeModeAssembly + 0x1c, &breakpointAddr, sizeof(breakpointAddr));
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
            pwrite(this->memoryFd, &breakpointCall, sizeof(breakpointCall), breakpointAddr);
        }
        Logger::getLogger().log(LogLevel::SUCCESS, "Done !");
        if(!ptrace(PTRACE_DETACH,this->pid,NULL,NULL)) {
            Logger::getLogger().log(LogLevel::SUCCESS, "Successfully detached from child process !");
            Logger::getLogger().log(LogLevel::INFO, "Entering pipe mode.");
            bool stop = false;
            while(!stop) {
                for(const filesystem::directory_entry &entry : filesystem::directory_iterator("./")) {
                    const string path = entry.path().u8string();
                    if(endsWith(toUppercase(path), ".HADES")) {
                        unsigned long long int pid = 0;
                        if(!inputToNumber("0x"+path.substr(path.length()-14, 8), pid)) {
                            Logger::getLogger().log(LogLevel::ERROR, "Unable to read breakpoint PID !");
                        }
                        unsigned long long int rip = this->readReg(pid, Register::RIP);
                        stringstream breakpointHit;
                        breakpointHit << "Breakpoint hit !" << endl;
                        breakpointHit << "PID: \033[;37m"  << hex << +pid << "\033[;36m" << endl;
                        breakpointHit << "Address: \033[;37m" << hex << +rip << "\033[;36m";
                        Logger::getLogger().log(LogLevel::INFO, breakpointHit.str());
                        stop = this->listenInput(pid);
                        if(stop) break;
                    }
                }
                this_thread::sleep_for(chrono::milliseconds(500));
            }
            Logger::getLogger().log(LogLevel::INFO, "Ending process...");
        } else Logger::getLogger().log(LogLevel::FATAL, "Failed to detach from child process...");
        close(this->memoryFd);
    } else Logger::getLogger().log(LogLevel::FATAL, "Unable to access memory of running target. Missing rights ?");
    this->handleExit();
}