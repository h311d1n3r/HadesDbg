#include <debugger.h>
#include <log.h>
#include <iomanip>
#include <cstring>
#include <thread>
#include <chrono>
#include <sys/wait.h>
#include <unistd.h>
#include <filesystem>
#include <utils.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <asmjit/asmjit.h>

using Code = HadesDbg::DbgCode;
using namespace std;
using namespace asmjit::x86;

bool HadesDbg::readBinaryHeader() {
    ifstream binaryRead(this->params.binaryPath, fstream::binary);
    if(binaryRead) {
        binaryRead.seekg(0, fstream::end);
        long len = binaryRead.tellg();
        binaryRead.seekg(0, fstream::beg);
        if(len >= 0x20) {
            char* idField = (char*) malloc(0x20);
            binaryRead.read(idField, 0x20);
            if(idField[0] == 0x7f && idField[1] == 'E' && idField[2] == 'L' && idField[3] == 'F') {
                if(idField[4] == 2) {
                    if(idField[5] == 1) {
                        if(idField[7] == 0 || idField[7] == 3) {
                            this->replacedFileEntry = (unsigned char*) malloc(this->injectPipeModeAssemblyVec.size());
                            this->fileEntry = *((BigInt*)(idField + 0x18));
                            if(this->fileEntry >= 0x400000) this->fileEntry -= 0x400000;
                            binaryRead.seekg((long)this->fileEntry, fstream::beg);
                            binaryRead.read((char*)this->replacedFileEntry, (long)this->injectPipeModeAssemblyVec.size());
                            binaryRead.close();
                            ofstream binaryWrite;
                            binaryWrite.open(this->params.binaryPath, fstream::binary | fstream::out | fstream::in);
                            binaryWrite.seekp((long)this->fileEntry, fstream::beg);
                            const char breakpoint = (const char) 0xCC;
                            binaryWrite.write(&breakpoint, 1);
                            stringstream msg;
                            msg << "Replaced byte 0x" << hex << setfill('0') << setw(2) << +this->replacedFileEntry[0] << " at address 0x" << hex << setfill('0') << setw(8) << +this->fileEntry << " (entry point) with byte 0xcc (breakpoint).";
                            Logger::getLogger().log(LogLevel::INFO, msg.str());
                            binaryWrite.close();
                            return true;
                        } else this->reportFatalError("The specified file can't be run on a Linux architecture.");
                    } else this->reportFatalError("HadesDbg currently handles little endian files only.");
                } else this->reportFatalError("HadesDbg currently handles x64 files only.");
            } else this->reportFatalError("The specified binary is not a ELF file.");
        } else {
            stringstream error;
            error << "The specified binary file is only " << len << " bytes long.";
            this->reportFatalError(error.str());
        }
        binaryRead.close();
    } else this->reportFatalError("An error occured while trying to open the specified binary file.");
    return false;
}

void HadesDbg::convertArgs(const vector<string>& args, char** execArgs) {
    int argsAmount = 0;
    for (const string& arg : args) {
        execArgs[argsAmount++] = strdup(arg.c_str());
    }
    execArgs[argsAmount++] = nullptr;
}

void HadesDbg::fixEntryBreakpoint() {
    this->fixedEntryBreakpoint = true;
    ofstream binaryWrite;
    do {
        binaryWrite.open(this->params.binaryPath, fstream::binary | fstream::out | fstream::in);
        this_thread::sleep_for(std::chrono::milliseconds(10));
    } while(!binaryWrite.is_open());
    binaryWrite.seekp((long)this->fileEntry, fstream::beg);
    binaryWrite.write((char*)this->replacedFileEntry, 1);
    binaryWrite.close();
    Logger::getLogger().log(LogLevel::INFO, "Restored breakpoint byte.");
}

void HadesDbg::handleExit() {
    if(this->params.outputFile && this->params.outputFile->is_open()) this->params.outputFile->close();
    stringstream lsofCommand;
    lsofCommand << "lsof -w \"" << this->params.binaryPath << "\"";
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
                BigInt sonPid = 0;
                if(inputToNumber(pidStr, sonPid)) {
                    stringstream killStr;
                    killStr << "Killing son with PID: \033[;37m" << hex << +sonPid << "\033[;36m";
                    Logger::getLogger().log(LogLevel::INFO, killStr.str());
                    kill((pid_t)sonPid, SIGKILL);
                }
            }
        }
    }
    for(const filesystem::directory_entry &entry : filesystem::directory_iterator("./")) {
        const string path = entry.path().u8string();
        if (endsWith(toUppercase(path), ".HADES")) {
            BigInt sonPid = 0;
            if (!inputToNumber("0x" + path.substr(path.length() - 14, 8), sonPid)) this->reportError("Unable to read breakpoint PID !");
            //Check if process with pid exists
            if(!kill((int)sonPid, 0)) {
                stringstream killStr;
                killStr << "Killing son with PID: \033[;37m" << hex << +sonPid << "\033[;36m";
                Logger::getLogger().log(LogLevel::INFO, killStr.str());
                kill((int)sonPid, SIGKILL);
            }
            remove(path.c_str());
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

BigInt HadesDbg::readReg(pid_t sonPid, Register reg) {
    const char readRegBytes[] = {
        Code::READ_REG,
        (char)(0x80 - reg), 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    };
    string filePath = this->prepareAction(sonPid, (char*)readRegBytes, sizeof(readRegBytes));
    char* buffer = (char*) malloc(0xa);
    while(true) {
        ifstream fileReader;
        fileReader.open(filePath, ios::binary | ios::in);
        fileReader.read(buffer, 0xa);
        fileReader.close();
        if(buffer[0] == Code::TARGET_READY) break;
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    BigInt ret = 0;
    memcpy(&ret, buffer+0x1, sizeof(ret));
    free(buffer);
    if(reg == Register::RIP) ret = ret - this->effectiveEntry + this->params.entryAddress;
    return ret;
}

BigInt HadesDbg::readMem(pid_t sonPid, BigInt addr) {
    char readMemBytes[] = {
        Code::READ_MEM,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    };
    memcpy(readMemBytes + 1, &addr, sizeof(addr));
    string filePath = this->prepareAction(sonPid, (char*)readMemBytes, sizeof(readMemBytes));
    char* buffer = (char*) malloc(0xa);
    while(true) {
        ifstream fileReader;
        fileReader.open(filePath, ios::binary | ios::in);
        fileReader.read(buffer, 0xa);
        fileReader.close();
        if(buffer[0] == Code::TARGET_READY) break;
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    BigInt ret = 0;
    memcpy(&ret, buffer+0x1, sizeof(ret));
    free(buffer);
    return ret;
}

void HadesDbg::writeReg(pid_t sonPid, Register reg, BigInt val) {
    char writeRegBytes[] = {
        Code::WRITE_REG,
        (char)(0x80 - reg), 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    };
    memcpy(writeRegBytes + 9, &val, sizeof(val));
    string filePath = this->prepareAction(sonPid, (char*)writeRegBytes, sizeof(writeRegBytes));
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

void HadesDbg::writeMem(pid_t sonPid, BigInt addr, BigInt val) {
    char writeMemBytes[] = {
            Code::WRITE_MEM,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    };
    memcpy(writeMemBytes + 1, &addr, sizeof(addr));
    memcpy(writeMemBytes + 9, &val, sizeof(val));
    string filePath = this->prepareAction(sonPid, (char*)writeMemBytes, sizeof(writeMemBytes));
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

map<string, BigInt> HadesDbg::readRegs(pid_t sonPid) {
    const char readRegsBytes[] = {
            Code::READ_REGS,
    };
    string filePath = this->prepareAction(sonPid, (char*)readRegsBytes, sizeof(readRegsBytes));
    char* buffer = (char*) malloc(0x2 + registerFromName.size() * 0x8);
    while(true) {
        ifstream fileReader;
        fileReader.open(filePath, ios::binary | ios::in);
        fileReader.read(buffer, 0x2 + registerFromName.size() * 0x8);
        fileReader.close();
        if(buffer[0] == Code::TARGET_READY) break;
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    map<string, BigInt> regs;
    unsigned char counter = 0;
    for(string regName : orderedRegsNames) {
        BigInt regVal = 0;
        memcpy(&regVal, buffer + 0x1 + (registerFromName.size() - counter - 1) * sizeof(regVal), sizeof(regVal));
        if(registerFromName[regName] == Register::RIP) regVal = regVal - this->effectiveEntry + this->params.entryAddress;
        regs[regName] = regVal;
        counter++;
    }
    free(buffer);
    return regs;
}

bool HadesDbg::endBp(pid_t sonPid) {
    const char endBpBytes[] = {
        Code::END_BREAKPOINT,
    };
    struct stat fileInfo;
    BigInt baseTime = time(&fileInfo.st_ctime);
    string filePath = this->prepareAction(sonPid, (char*)endBpBytes, sizeof(endBpBytes));
    unsigned int counter = 0;
    unsigned int pause = 500;
    unsigned int timeout = 5 * 1000;
    int res = 0;
    while(stat(filePath.c_str(), &fileInfo) == 0 && baseTime >= time(&fileInfo.st_ctime)) {
        this_thread::sleep_for(chrono::milliseconds(pause));
        if(counter * pause > timeout) return false;
        counter++;
    }
    return true;
}

vector<unsigned char> HadesDbg::preparePipeModeAssembly() {
    asmjit::JitRuntime rt;
    asmjit::CodeHolder code;
    code.init(rt.environment());
    Assembler a(&code);

    //restore rax
    a.pop(rax);
    a.pop(rax);

    //push registers
    a.push(rsp);
    a.push(rax);
    a.push(rbx);
    a.push(rcx);
    a.push(rdx);
    a.push(rdi);
    a.push(rsi);
    a.push(r8);
    a.push(r9);
    a.push(r10);
    a.push(r11);
    a.push(r12);
    a.push(r13);
    a.push(r14);
    a.push(r15);
    a.push(rbp);

    //push rax
    a.movabs(rax, 0);
    a.push(rax);

    //allocate space for pipe file name (./pipe_XXXXXXXX.hades0).
    //start of allocated region is moved to rbx
    a.mov(rax, 9);
    a.mov(rdi, 0);
    a.mov(rsi, 0x16);
    a.mov(rdx, 7);
    a.mov(r10, 0x22);
    a.mov(r8, -1);
    a.mov(r9, 0);
    a.syscall();
    a.mov(rbx, rax);

    //get pid
    a.mov(rax, 0x27);
    a.syscall();

    //write pipe file name in allocated region
    //start of allocated region is moved to rdi
    a.mov(rdi, rbx);
    a.mov(qword_ptr(rbx), 0x69702f2e);
    a.add(rbx, 4);
    a.mov(byte_ptr(rbx), 0x70);
    a.inc(rbx);
    a.mov(byte_ptr(rbx), 0x65);
    a.inc(rbx);
    a.mov(byte_ptr(rbx), 0x5f);
    a.mov(rdx, rbx);
    a.add(rbx, 8);
    asmjit::Label nameLoopLabel = a.newLabel();
    a.bind(nameLoopLabel);
    a.mov(rcx, rax);
    a.and_(rcx, 0xf);
    a.cmp(rcx, 9);
    asmjit::Label letterLabel = a.newLabel();
    asmjit::Label afterLetterLabel = a.newLabel();
    a.ja(letterLabel);
    a.add(rcx, 0x30);
    a.jmp(afterLetterLabel);
    a.bind(letterLabel);
    a.add(rcx, 0x57);
    a.bind(afterLetterLabel);
    a.mov(byte_ptr(rbx), cl);
    a.dec(rbx);
    a.shr(rax, 4);
    a.cmp(rbx, rdx);
    a.jne(nameLoopLabel);
    a.add(rbx, 9);
    a.mov(qword_ptr(rbx), 0x6461682e);
    a.add(rbx, 4);
    a.mov(byte_ptr(rbx), 0x65);
    a.inc(rbx);
    a.mov(byte_ptr(rbx), 0x73);
    a.inc(rbx);
    a.mov(byte_ptr(rbx), 0);

    //open pipe file in create mode
    //start of allocated region is pushed on the stack
    //file descriptor is in rax
    a.mov(rax, 2);
    a.mov(rsi, 0x42);
    a.mov(rdx, 0x1ff);
    a.syscall();
    a.push(rdi);

    //allocate region for read and write operations
    //file descriptor is moved to rbx
    //start of allocation is in rax
    a.mov(rbx, rax);
    a.mov(rax, 9);
    a.mov(rdi, 0);
    a.mov(rsi, 0xff);
    a.mov(rdx, 7);
    a.mov(r10, 0x22);
    a.mov(r8, -1);
    a.mov(r9, 0);
    a.syscall();

    //write target ready message to file
    //start of allocation is moved to rsi
    a.mov(rsi, rax);
    a.mov(byte_ptr(rax), Code::TARGET_READY);
    a.mov(rdi, rbx);
    a.mov(rax, 1);
    a.mov(rdx, 1);
    a.syscall();

    //close file
    a.mov(rax, 3);
    a.syscall();

    //wait for 1 second
    //start of allocation is put in r8
    asmjit::Label timeLabel = a.newLabel();
    a.bind(timeLabel);
    a.mov(rdx, rsi);
    a.mov(qword_ptr(rdx), 1);
    a.add(rdx, 4);
    a.mov(qword_ptr(rdx), 0);
    a.add(rdx, 4);
    a.mov(qword_ptr(rdx), 0);
    a.add(rdx, 4);
    a.mov(qword_ptr(rdx), 0);
    a.mov(rdi, rsi);
    a.mov(r8, rdi);
    a.mov(rsi, 8);
    a.mov(rax, 0x23);
    a.syscall();

    //open pipe file again
    //if it fails, jump back to wait
    a.pop(rdi);
    a.push(rdi);
    a.mov(rax, 2);
    a.mov(rsi, 2);
    a.mov(rdx, 0x1ff);
    a.syscall();
    a.mov(rsi, r8);
    a.cmp(rax, 0);
    a.js(timeLabel);

    //read in allocated region
    //if target ready message, jump back
    //file descriptor is in rbx
    a.mov(rdi, rax);
    a.mov(rsi, r8);
    a.mov(rdx, 0xff);
    a.mov(rax, 0);
    a.syscall();
    a.cmp(byte_ptr(r8), Code::TARGET_READY);
    a.je(timeLabel);
    a.mov(rbx, rdi);

    //read register
    a.cmp(byte_ptr(r8), Code::READ_REG);
    asmjit::Label readMemLabel = a.newLabel();
    a.jne(readMemLabel);
    a.add(rsp, 8);
    a.inc(r8);
    a.add(rsp, qword_ptr(r8));
    a.pop(r9);
    a.sub(rsp, qword_ptr(r8));
    a.dec(r8);
    a.sub(rsp, 0x10);
    a.mov(byte_ptr(r8), Code::TARGET_READY);
    a.inc(r8);
    a.mov(qword_ptr(r8), r9);
    a.add(r8, 8);
    a.mov(byte_ptr(r8), 0);
    a.sub(r8, 9);
    a.mov(rdi, rbx);
    a.mov(rsi, 0);
    a.mov(rdx, 0);
    a.mov(rax, 8);
    a.syscall();
    a.mov(rsi, r8);
    a.mov(rdx, 0xa);
    a.mov(rax, 1);
    a.syscall();

    //read memory value
    a.bind(readMemLabel);
    a.cmp(byte_ptr(r8), Code::READ_MEM);
    asmjit::Label writeRegLabel = a.newLabel();
    a.jne(writeRegLabel);
    a.inc(r8);
    a.mov(r9, qword_ptr(r8));
    a.mov(r9, qword_ptr(r9));
    a.dec(r8);
    a.mov(byte_ptr(r8), Code::TARGET_READY);
    a.inc(r8);
    a.mov(qword_ptr(r8), r9);
    a.add(r8, 8);
    a.mov(byte_ptr(r8), 0);
    a.sub(r8, 9);
    a.mov(rdi, rbx);
    a.mov(rsi, 0);
    a.mov(rdx, 0);
    a.mov(rax, 8);
    a.syscall();
    a.mov(rsi, r8);
    a.mov(rdx, 0xa);
    a.mov(rax, 1);
    a.syscall();

    //write register
    a.bind(writeRegLabel);
    a.cmp(byte_ptr(r8), Code::WRITE_REG);
    asmjit::Label writeMemLabel = a.newLabel();
    a.jne(writeMemLabel);
    a.add(rsp, 0x10);
    a.inc(r8);
    a.add(rsp, qword_ptr(r8));
    a.add(r8, 8);
    a.push(qword_ptr(r8));
    a.sub(r8, 8);
    a.sub(rsp, 8);
    a.sub(rsp, qword_ptr(r8));
    a.dec(r8);
    a.mov(byte_ptr(r8), Code::TARGET_READY);
    a.inc(r8);
    a.mov(byte_ptr(r8), 0);
    a.dec(r8);
    a.mov(rdi, rbx);
    a.mov(rsi, 0);
    a.mov(rdx, 0);
    a.mov(rax, 8);
    a.syscall();
    a.mov(rsi, r8);
    a.mov(rdx, 2);
    a.mov(rax, 1);
    a.syscall();

    //write memory
    a.bind(writeMemLabel);
    a.cmp(byte_ptr(r8), Code::WRITE_MEM);
    asmjit::Label readRegsLabel = a.newLabel();
    a.jne(readRegsLabel);
    a.inc(r8);
    a.mov(r9, qword_ptr(r8));
    a.add(r8, 8);
    a.mov(r10, qword_ptr(r8));
    a.sub(r8, 9);
    a.mov(qword_ptr(r9), r10);
    a.mov(byte_ptr(r8), Code::TARGET_READY);
    a.inc(r8);
    a.mov(byte_ptr(r8), 0);
    a.dec(r8);
    a.mov(rdi, rbx);
    a.mov(rsi, 0);
    a.mov(rdx, 0);
    a.mov(rax, 8);
    a.syscall();
    a.mov(rsi, r8);
    a.mov(rdx, 2);
    a.mov(rax, 1);
    a.syscall();

    //read registers
    a.bind(readRegsLabel);
    a.cmp(byte_ptr(r8), Code::READ_REGS);
    asmjit::Label closePipeLabel = a.newLabel();
    a.jne(closePipeLabel);
    a.mov(byte_ptr(r8), Code::TARGET_READY);
    a.inc(r8);
    a.add(rsp, 8);
    a.xor_(rcx, rcx);
    asmjit::Label loopReadRegsLabel = a.newLabel();
    a.bind(loopReadRegsLabel);
    a.pop(r9);
    a.mov(qword_ptr(r8), r9);
    a.add(r8, 8);
    a.inc(rcx);
    a.cmp(rcx, registerFromName.size());
    a.jb(loopReadRegsLabel);
    a.mov(byte_ptr(r8), 0x0);
    a.sub(rsp, 0x8 + registerFromName.size() * 0x8);
    a.sub(r8, 0x1 + registerFromName.size() * 0x8);
    a.mov(rdi, rbx);
    a.mov(rsi, 0);
    a.mov(rdx, 0);
    a.mov(rax, 8);
    a.syscall();
    a.mov(rsi, r8);
    a.mov(rdx, 0x2 + registerFromName.size() * 8);
    a.mov(rax, 1);
    a.syscall();

    //close pipe file
    a.bind(closePipeLabel);
    a.mov(rax, 3);
    a.mov(rdi, rbx);
    a.syscall();

    //check end_breakpoint
    a.cmp(byte_ptr(r8), Code::END_BREAKPOINT);
    a.jne(timeLabel);

    //delete pipe file
    //pipe file name is in rdi
    a.pop(rdi);
    a.mov(rax, 0x57);
    a.syscall();

    //free read/write region
    a.mov(rax, 0xb);
    a.mov(rdi, r8);
    a.mov(rsi, 0xff);
    a.syscall();

    //free pipe file name region
    a.mov(rax, 0xb);
    a.mov(rsi, 0x16);
    a.syscall();

    //prevent pop of rip
    a.add(rsp, 8);

    //pop registers
    a.pop(rbp);
    a.pop(r15);
    a.pop(r14);
    a.pop(r13);
    a.pop(r12);
    a.pop(r11);
    a.pop(r10);
    a.pop(r9);
    a.pop(r8);
    a.pop(rsi);
    a.pop(rdi);
    a.pop(rdx);
    a.pop(rcx);
    a.pop(rbx);
    a.pop(rax);

    //prevent pop of rsp
    a.add(rsp, 8);

    //replaced instructions
    for(unsigned char i = 0; i < 64; i++) a.nop();

    //return
    a.sub(rsp, 8);
    a.push(rax);
    a.add(rsp, 0x10);
    a.movabs(rax, 0);
    a.push(rax);
    a.sub(rsp, 8);
    a.pop(rax);
    a.ret();

    vector<unsigned char> vec(a.bufferData(), a.bufferPtr());
    return vec;
}

vector<unsigned char> HadesDbg::preparePipeModeAssemblyInjection(vector<unsigned char> pipeModeAssemblyVec) {
    asmjit::JitRuntime rt;
    asmjit::CodeHolder code;
    code.init(rt.environment());
    Assembler a(&code);

    a.mov(rax, 0x9);
    a.mov(rdi, 0x0);
    a.mov(rsi, 0x0);
    a.mov(rdx, 0x7);
    a.mov(r10, 0x22);
    a.mov(r8, -0x1);
    a.mov(r9, 0x0);
    a.syscall();

    vector<unsigned char> vec(a.bufferData(), a.bufferPtr());
    vec.push_back(0x50);
    unsigned char movArr[] = {0x48,0xc7,0x00};
    unsigned char addArr[] = {0x48,0x83,0xc0,0x04};
    for(int i = 0; i < pipeModeAssemblyVec.size(); i+=4) {
        vec.insert(vec.end(), movArr, movArr + sizeof(movArr));
        for(int i2 = i; i2 < i + 4; i2++) {
            if(i2 < pipeModeAssemblyVec.size()) vec.push_back(pipeModeAssemblyVec[i2]);
            else vec.push_back(0x0);
        }
        if(i + 4 < pipeModeAssemblyVec.size()) {
            vec.insert(vec.end(), addArr, addArr + sizeof(addArr));
        }
    }
    vec.push_back(0x58);
    vec.push_back(0xcc);
    return vec;
}

void HadesDbg::execCommand(pid_t sonPid, string input) {
    vector<string> cmdParams;
    split(input, ' ', cmdParams);
    if(!cmdParams.empty()) {
        if(cmdParams[0] == "readreg" || cmdParams[0] == "rr") {
            if(cmdParams.size() > 1) {
                string regName = toUppercase(cmdParams[1]);
                if (registerFromName.count(regName)) {
                    stringstream regAskStr;
                    regAskStr << "Asking \033[;37m" << hex << sonPid << "\033[;36m for \033[;37m" << regName << " \033[;36mvalue...";
                    Register reg = registerFromName[regName];
                    Logger::getLogger().log(LogLevel::INFO, regAskStr.str());
                    BigInt regValue = readReg(sonPid, reg);
                    stringstream regValStr;
                    regValStr << regName << ": \033[;37m" << hex << regValue << "\033[;32m";
                    Logger::getLogger().log(LogLevel::SUCCESS, regValStr.str());
                    if(this->params.outputFile && this->params.outputFile->is_open()) {
                        *this->params.outputFile << removeConsoleChars(regValStr.str()) << endl;
                    }
                } else Logger::getLogger().log(LogLevel::WARNING, "First parameter must be a valid register ! \033[;37mreadreg register\033[;33m");
            } else Logger::getLogger().log(LogLevel::WARNING, "This command requires more parameters ! \033[;37mreadreg register\033[;33m");
        }
        else if(cmdParams[0] == "readmem" || cmdParams[0] == "rm" || cmdParams[0] == "readmem_ascii" || cmdParams[0] == "rma") {
            if(cmdParams.size() > 2) {
                string addrStr = cmdParams[1];
                bool entryRelative = false;
                bool ascii = cmdParams[0] == "readmem_ascii" || cmdParams[0] == "rma";
                if(!addrStr.find('@')) {
                    entryRelative = true;
                    addrStr = addrStr.substr(1);
                }
                BigInt addr = 0;
                if(inputToNumber(addrStr, addr)) {
                    string lenStr = cmdParams[2];
                    BigInt len = 0;
                    if(inputToNumber(lenStr, len)) {
                        if(len > 0) {
                            stringstream memAskStr;
                            memAskStr << "Asking \033[;37m" << hex << sonPid << "\033[;36m for \033[;37m" << hex << len
                                      << " \033[;36mbytes at address \033[;37m" << hex << addr << "\033[;36m...";
                            Logger::getLogger().log(LogLevel::INFO, memAskStr.str());
                            stringstream memValStr;
                            memValStr << hex << addr << ":";
                            Logger::getLogger().log(LogLevel::SUCCESS, memValStr.str());
                            stringstream output;
                            for(int i = 0; i < len; i+=8) {
                                BigInt memValue = readMem(sonPid, entryRelative ? (addr + i - this->params.entryAddress + this->effectiveEntry) : addr + i);
                                for(int i2 = 0; i2 < sizeof(memValue) && i+i2 < len; i2++) {
                                    if(!ascii) {
                                        output << hex << setfill('0') << setw(2) << +(memValue >> (i2 * 8) & 0xff);
                                        if(i2+1 < sizeof(memValue) && i+i2+1 < len) output << " ";
                                    } else {
                                        output << (char)(memValue >> (i2 * 8) & 0xff);
                                    }
                                }
                                if(!ascii) cout << output.str() << endl;
                                else cout << output.str();
                                if(this->params.outputFile && this->params.outputFile->is_open()) {
                                    if(!ascii) *this->params.outputFile << removeConsoleChars(output.str()) << endl;
                                    else *this->params.outputFile << removeConsoleChars(output.str());
                                }
                                output.str("");
                            }
                            if(ascii) {
                                cout << endl;
                                if(this->params.outputFile && this->params.outputFile->is_open()) *this->params.outputFile << endl;
                            }
                            Logger::getLogger().log(LogLevel::SUCCESS, "Done !");
                        } else Logger::getLogger().log(LogLevel::WARNING, "Length must be a strictly positive value ! \033[;37mreadmem address length\033[;33m");
                    } else Logger::getLogger().log(LogLevel::WARNING, "Second parameter must be a valid length ! \033[;37mreadmem address length\033[;33m");
                } else Logger::getLogger().log(LogLevel::WARNING, "First parameter must be a valid address ! \033[;37mreadmem address length\033[;33m");
            } else Logger::getLogger().log(LogLevel::WARNING, "This command requires more parameters ! \033[;37mreadmem address length\033[;33m");
        }
        else if(cmdParams[0] == "writereg" || cmdParams[0] == "wr") {
            if(cmdParams.size() > 2) {
                string regName = toUppercase(cmdParams[1]);
                if (registerFromName.count(regName)) {
                    Register reg = registerFromName[regName];
                    string valStr = cmdParams[2];
                    BigInt val;
                    if(inputToNumber(valStr, val)) {
                        stringstream regEditStr;
                        regEditStr << "Asking \033[;37m" << hex << sonPid << "\033[;36m to set \033[;37m" << regName
                                   << "\033[;36m to \033[;37m" << hex << val << "\033[;36m...";
                        Logger::getLogger().log(LogLevel::INFO, regEditStr.str());
                        this->writeReg(sonPid, reg, val);
                        Logger::getLogger().log(LogLevel::SUCCESS, "Success !");
                    } else Logger::getLogger().log(LogLevel::WARNING, "Second parameter must be a number ! \033[;37mwritereg register value\033[;33m");
                } else Logger::getLogger().log(LogLevel::WARNING, "First parameter must be a valid register ! \033[;37mwritereg register value\033[;33m");
            } else Logger::getLogger().log(LogLevel::WARNING, "This command requires more parameters ! \033[;37mwritereg register value\033[;33m");
        }
        else if(cmdParams[0] == "writemem" || cmdParams[0] == "wm") {
            if(cmdParams.size() > 2) {
                string addrStr = cmdParams[1];
                bool entryRelative = false;
                if(!addrStr.find('@')) {
                    entryRelative = true;
                    addrStr = addrStr.substr(1);
                }
                BigInt addr;
                if(inputToNumber(addrStr, addr)) {
                    string hexChain = cmdParams[2];
                    stringstream memEditStr;
                    memEditStr << "Asking \033[;37m" << hex << sonPid << "\033[;36m to write \033[;37m" << hexChain
                               << "\033[;36m at \033[;37m" << hex << addr << "\033[;36m...";
                    Logger::getLogger().log(LogLevel::INFO, memEditStr.str());
                    for(int i = 0; i < hexChain.length(); i += 2 * sizeof(BigInt)) {
                        BigInt val = 0;
                        unsigned long substrLen = i + 2 * sizeof(BigInt) > hexChain.length() ? hexChain.length() - i : 2 * sizeof(BigInt);
                        if(inputToNumber("0x"+hexChain.substr(i, substrLen), val)) {
                            val = invertEndian(val);
                            if(substrLen < 2 * sizeof(BigInt)) {
                                BigInt oldVal = readMem(sonPid, entryRelative ? (addr + (i / 2) - this->params.entryAddress + this->effectiveEntry) : addr + (i / 2));
                                val >>= sizeof(BigInt) * 8 - (substrLen * 8 / 2);
                                val += (oldVal >> (substrLen * 8 / 2)) << (substrLen * 8 / 2);
                            }
                            this->writeMem(sonPid, entryRelative ? (addr + (i / 2) - this->params.entryAddress + this->effectiveEntry) : addr + (i / 2), val);
                        } else Logger::getLogger().log(LogLevel::WARNING, "Second parameter must be a valid hexadecimal chain ! \033[;37mwritemem address hex_chain\033[;33m");
                    }
                    Logger::getLogger().log(LogLevel::SUCCESS, "Success !");
                } else Logger::getLogger().log(LogLevel::WARNING, "First parameter must be a valid address ! \033[;37mwritemem address hex_chain\033[;33m");
            } else Logger::getLogger().log(LogLevel::WARNING, "This command requires more parameters ! \033[;37mwritemem address hex_chain\033[;33m");
        } else if(cmdParams[0] == "readregs" || cmdParams[0] == "rrs") {
            stringstream regsAskStr;
            regsAskStr << "Asking \033[;37m" << hex << sonPid << "\033[;36m for \033[;37mall registers\033[;36m...";
            Logger::getLogger().log(LogLevel::INFO, regsAskStr.str());
            map<string, BigInt> regs = readRegs(sonPid);
            Logger::getLogger().log(LogLevel::SUCCESS, "");
            stringstream regsStr;
            unsigned char counter = 0;
            for(string regName : orderedRegsNames) {
                regsStr << regName << ":" << (regName.length() == 2 ? " " : "") << " \033[;37m" << setfill('0') << setw(2 * sizeof(BigInt)) << hex << regs[regName] << "\033[;32m";
                if(counter < regs.size() - 1) {
                    if(((counter + 1) % 3) == 0) regsStr << endl;
                    else regsStr << "    ";
                }
                counter++;
            }
            Logger::getLogger().log(LogLevel::SUCCESS, regsStr.str(), false, false);
            if(this->params.outputFile && this->params.outputFile->is_open()) {
                *this->params.outputFile << removeConsoleChars(regsStr.str()) << endl;
            }
        } else if(cmdParams[0] == "info" || cmdParams[0] == "i") {
            stringstream breakpointsStr;
            map<BigInt, unsigned char>::iterator breakpoint;
            unsigned char counter = 0;
            for(breakpoint = this->params.breakpoints.begin(); breakpoint != this->params.breakpoints.end(); breakpoint++) {
                breakpointsStr << "\033[;37m0x" << hex << +breakpoint->first << "\033[;36m";
                if(counter < this->params.breakpoints.size() - 1) breakpointsStr << ", ";
                counter++;
            }
            stringstream infoStr;
            infoStr << "Debug info:" << endl
                    << "File: " << "\033[;37m" << this->params.binaryPath << "\033[;36m" << endl
                    << "Process ID (original): " << "\033[;37m0x" << hex << +this->pid << "\033[;36m" << endl
                    << "Process ID (current): " << "\033[;37m0x" << hex << +sonPid << "\033[;36m" << endl
                    << "Entry point (ELF header): " << "\033[;37m0x" << hex << +this->fileEntry << "\033[;36m" << endl
                    << "Entry point (effective): " << "\033[;37m0x" << hex << +this->effectiveEntry << "\033[;36m" << endl
                    << "Entry point (defined): " << "\033[;37m0x" << hex << +this->params.entryAddress << "\033[;36m" << endl
                    << "Effective offset: displayedAddress + " << "\033[;37m0x" << hex << +(this->effectiveEntry - this->params.entryAddress) << "\033[;36m" << endl
                    << "Breakpoints: " << breakpointsStr.str();
            Logger::getLogger().log(LogLevel::INFO, infoStr.str());
        } else Logger::getLogger().log(LogLevel::WARNING, "Unknown command !");
    } else Logger::getLogger().log(LogLevel::WARNING, "Please input a command !");
}

bool HadesDbg::listenInput(pid_t sonPid) {
    string input;
    while(true) {
        getline(cin, input);
        if(input == "exit" || input == "e") return true;
        else if(input == "run" || input == "r") {
            stringstream runAskStr;
            runAskStr << "Asking \033[;37m" << hex << +sonPid << "\033[;36m to resume execution...";
            Logger::getLogger().log(LogLevel::INFO, runAskStr.str());
            if(this->endBp(sonPid)) Logger::getLogger().log(LogLevel::SUCCESS, "Success !");
            else this->reportError("An error occured !");
            return false;
        } else this->execCommand(sonPid, input);
    }
    return true;
}

void HadesDbg::reportError(string error) {
    Logger::getLogger().log(LogLevel::ERROR, error);
    if(this->params.outputFile && this->params.outputFile->is_open()) *this->params.outputFile << "Error" << endl;
}

void HadesDbg::reportFatalError(string error) {
    Logger::getLogger().log(LogLevel::FATAL, error);
    if(this->params.outputFile && this->params.outputFile->is_open()) *this->params.outputFile << "Fatal error" << endl;
}

bool HadesDbg::readScriptFile() {
    string line;
    BigInt currentBp = -1;
    unsigned int lineNumber = 0;
    vector<string> currentVec;
    while (getline(*this->params.scriptFile, line)) {
        if(!line.rfind("bp ", 0)) {
            if(!currentVec.empty() && currentBp > 0) {
                this->scriptByBreakpoint[currentBp] = currentVec;
                currentVec.clear();
            }
            if(!inputToNumber(line.substr(string("bp ").length()), currentBp)) {
                stringstream msg;
                msg << "Error line \033[;37m" << +lineNumber << "\033[;31m : Breakpoint index must be a number !";
                this->reportFatalError(msg.str());
                return false;
            } else if(currentBp <= 0) {
                stringstream msg;
                msg << "Error line \033[;37m" << +lineNumber << "\033[;31m : Breakpoint index must be >= 1 !";
                this->reportFatalError(msg.str());
                return false;
            } else if(this->scriptByBreakpoint.count(currentBp)) {
                stringstream msg;
                msg << "Error line \033[;37m" << +lineNumber << "\033[;31m : Instructions have already been set for this breakpoint !";
                this->reportFatalError(msg.str());
                return false;
            }
        } else if(currentBp > 0) {
            if(line.find_first_not_of(' ') != string::npos && line.find_first_not_of('\t') != string::npos) {
                currentVec.push_back(line);
            }
        } else {
            stringstream msg;
            msg << "Error line \033[;37m" << +lineNumber << "\033[;31m : Breakpoint index was not set ! Use \"bp\" command before breakpoint instructions...";
            this->reportFatalError(msg.str());
            return false;
        }
        lineNumber++;
    }
    if(!currentVec.empty() && currentBp > 0) this->scriptByBreakpoint[currentBp] = currentVec;
    this->params.scriptFile->close();
    return true;
}

void HadesDbg::run() {
    if(!this->readBinaryHeader()) return;
    if(this->params.scriptFile && this->params.scriptFile->is_open()) {
        if(!this->readScriptFile()) return;
    }
    this->pid = fork();
    if(!this->pid) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        char* execArgs[1024];
        convertArgs(params.binaryArgs, execArgs);
        execv(this->params.binaryPath.c_str(), execArgs);
    }
    waitpid(this->pid, nullptr, 0);
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    //breakpoint set in binary
    waitpid(this->pid, nullptr, 0);
    char file[64];
    sprintf(file, "/proc/%ld/mem", (long)this->pid);
    this->memoryFd = open(file, O_RDWR);
    if(this->memoryFd != -1) {
        asmjit::JitRuntime rt;
        asmjit::CodeHolder code;
        code.init(rt.environment());
        Assembler a(&code);
        a.push(rax);
        a.movabs(rax, 0x0000000000000000);
        a.call(rax);
        vector<unsigned char> breakpointCallVec(a.bufferData(), a.bufferPtr());
        unsigned char* breakpointCall(&breakpointCallVec[0]);
        unsigned int breakpointCallSize = breakpointCallVec.size();
        user_regs_struct regs = {};
        Logger::getLogger().log(LogLevel::SUCCESS, "Target reached entry breakpoint !");
        Logger::getLogger().log(LogLevel::INFO, "Injecting pipe mode in child process...");
        ptrace(PTRACE_GETREGS,this->pid,NULL,&regs);
        regs.rip--;
        user_regs_struct savedRegs = regs;
        this->effectiveEntry = regs.rip;
        pwrite(this->memoryFd, this->replacedFileEntry, 1, (long)this->effectiveEntry);
        unsigned int injectPipeModeAssemblySize = this->injectPipeModeAssemblyVec.size();
        map<BigInt, BigInt> allocAddrFromBpAddr;
        unsigned int pipeModeAssemblySize = this->pipeModeAssemblyVec.size();
        map<BigInt, unsigned char>::iterator breakpoint;
        for(breakpoint = this->params.breakpoints.begin(); breakpoint != this->params.breakpoints.end(); breakpoint++) {
            BigInt breakpointAddr = breakpoint->first - this->params.entryAddress + this->effectiveEntry;
            unsigned char breakpointLength = breakpoint->second;
            pread(this->memoryFd, &this->pipeModeAssemblyVec[0] + pipeModeAssemblySize - 64 - 26, breakpointLength, (long)breakpointAddr);
            memcpy(&this->pipeModeAssemblyVec[0] + 0x1c, &breakpointAddr, sizeof(breakpointAddr));
            BigInt returnAddr = breakpointAddr + breakpointLength;
            memcpy(&this->pipeModeAssemblyVec[0] + pipeModeAssemblySize - 0xf, &returnAddr, sizeof(returnAddr));
            this->injectPipeModeAssemblyVec = preparePipeModeAssemblyInjection(this->pipeModeAssemblyVec);
            unsigned char *injectPipeModeAssembly = &(this->injectPipeModeAssemblyVec)[0];
            memcpy(injectPipeModeAssembly + 0x11, &pipeModeAssemblySize, sizeof(pipeModeAssemblySize));
            pwrite(this->memoryFd, injectPipeModeAssembly, injectPipeModeAssemblySize, (long)this->effectiveEntry);
            memset(&this->pipeModeAssemblyVec[0] + pipeModeAssemblySize - 64 - 26, 0x90, 64);
            ptrace(PTRACE_SETREGS, this->pid, NULL, &savedRegs);
            ptrace(PTRACE_CONT, pid, NULL, NULL);
            waitpid(this->pid, nullptr, 0);
            ptrace(PTRACE_GETREGS,this->pid,NULL,&regs);
            unsigned long long allocStart = regs.rax;
            if(!allocStart) {
                this->reportFatalError("Failure...");
                close(this->memoryFd);
                this->handleExit();
            }
            allocAddrFromBpAddr[breakpointAddr] = allocStart;
        }
        Logger::getLogger().log(LogLevel::SUCCESS, "Success !");
        ptrace(PTRACE_SETREGS, this->pid, NULL, &savedRegs);
        pwrite(this->memoryFd, this->replacedFileEntry, injectPipeModeAssemblySize, (long)this->effectiveEntry);
        map<BigInt, BigInt>::iterator injectData;
        stringstream injectBpMsg;
        injectBpMsg << "Injecting \033[;37m" << allocAddrFromBpAddr.size() << "\033[;36m breakpoint(s)...";
        Logger::getLogger().log(LogLevel::INFO, injectBpMsg.str());
        for(injectData = allocAddrFromBpAddr.begin(); injectData != allocAddrFromBpAddr.end(); injectData++) {
            BigInt breakpointAddr = injectData->first;
            BigInt allocStart = injectData->second;
            unsigned char breakpointSize = this->params.breakpoints.at(breakpointAddr - this->effectiveEntry + this->params.entryAddress);
            memcpy(breakpointCall + 0x3, &allocStart, sizeof(allocStart));
            pwrite(this->memoryFd, breakpointCall, breakpointCallSize, (long)breakpointAddr);
            unsigned char nopsAmount = breakpointSize - breakpointCallSize;
            char* nopsArr = (char*) malloc(nopsAmount);
            for(int i = 0; i < nopsAmount; i++) nopsArr[i] = 0x90;
            if(nopsArr) {
                pwrite(this->memoryFd, nopsArr, nopsAmount, (long)breakpointAddr + breakpointCallSize);
                delete nopsArr;
            } else {
                this->reportFatalError("Failure...");
                close(this->memoryFd);
                this->handleExit();
            }
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
                        BigInt sonPid = 0;
                        if(!inputToNumber("0x"+path.substr(path.length()-14, 8), sonPid)) {
                            this->reportError("Unable to read breakpoint PID !");
                        }
                        BigInt sonRip = this->readReg((int)sonPid, Register::RIP);
                        unsigned int bpIndex = this->params.bpIndexFromAddr[sonRip];
                        stringstream breakpointHit;
                        breakpointHit << "Breakpoint \033[;37m" << +bpIndex << "\033[;36m hit !" << endl;
                        breakpointHit << "PID: \033[;37m" << hex << +sonPid << "\033[;36m" << endl;
                        breakpointHit << "Address: \033[;37m" << hex << +sonRip << "\033[;36m";
                        Logger::getLogger().log(LogLevel::INFO, breakpointHit.str());
                        if(this->scriptByBreakpoint.count(bpIndex)) {
                            vector<string> commands = this->scriptByBreakpoint[bpIndex];
                            for(string command : commands) {
                                stringstream commandMsg;
                                commandMsg << "Script >> \033[;37m" << command << "\033[;36m";
                                Logger::getLogger().log(LogLevel::INFO, commandMsg.str());
                                if(command == "exit" || command == "e") {
                                    stop = true;
                                    break;
                                } else this->execCommand((int) sonPid, command);
                            }
                            if(!stop) {
                                stringstream runAskStr;
                                runAskStr << "Asking \033[;37m" << hex << +sonPid << "\033[;36m to resume execution...";
                                Logger::getLogger().log(LogLevel::INFO, runAskStr.str());
                                if(this->endBp(sonPid)) Logger::getLogger().log(LogLevel::SUCCESS, "Success !");
                                else this->reportError("An error occured !");
                            }
                        }
                        else {
                            stop = this->listenInput((int) sonPid);
                        }
                        if (stop) break;
                    }
                }
                this_thread::sleep_for(chrono::milliseconds(500));
            }
            Logger::getLogger().log(LogLevel::INFO, "Ending process...");
        } else this->reportFatalError("Failed to detach from child process...");
        close(this->memoryFd);
    } else this->reportFatalError("Unable to access memory of running target. Missing rights ?");
    this->handleExit();
}