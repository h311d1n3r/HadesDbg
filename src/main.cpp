#include <iostream>
#include <string>
#include <log.h>
#include <csignal>
#include <unistd.h>
#include <filesystem>
#include <cstring>
#include <vector>
#include <debugger.h>

using namespace std;

const string TOOL_TITLE = "  _  _         _        ___  _          \n"
                          " | || |__ _ __| |___ __|   \\| |__  __ _ \n"
                          " | __ / _` / _` / -_|_-< |) | '_ \\/ _` |\n"
                          " |_||_\\__,_\\__,_\\___/__/___/|_.__/\\__, |\n"
                          "                                  |___/ ";
const string VERSION = "1.1";
const string AUTHOR = "h311d1n3r";
#if __x86_64__
const string ARCHITECTURE = "64bit";
#else
const string ARCHITECTURE = "32bit";
#endif

BinaryParams params;
HadesDbg* dbg;
unsigned int bpIndex = 1;

void printHelpMessage() {
    Logger::getLogger().log(LogLevel::INFO, TOOL_TITLE, false, false);
    stringstream version;
    version << " Version: " << Logger::getLogger().getLogColorStr(LogLevel::VARIABLE, true) + VERSION;
    Logger::getLogger().log(LogLevel::INFO, version.str(), false, false);
    stringstream author;
    author << " Author: " << Logger::getLogger().getLogColorStr(LogLevel::VARIABLE, true) + AUTHOR;
    Logger::getLogger().log(LogLevel::INFO, author.str(), false, false);
    stringstream arch;
    arch << " Architecture: " << Logger::getLogger().getLogColorStr(LogLevel::VARIABLE, true) + ARCHITECTURE;
    Logger::getLogger().log(LogLevel::INFO, arch.str(), false, false);
    Logger::getLogger().log(LogLevel::INFO, "________________________________________\n", false, false);
    Logger::getLogger().log(LogLevel::INFO, "Syntax: " + Logger::getLogger().getLogColorStr(LogLevel::WARNING, true) + "hadesdbg binary [-param value] [--flag]\n", false, false);
    Logger::getLogger().log(LogLevel::INFO, "Parameters:", false, false);
    stringstream paramsList;
    paramsList << Logger::getLogger().getLogColorStr(LogLevel::WARNING, true) + "   entry" + Logger::getLogger().getLogColorStr(LogLevel::INFO) + " -> Specifies the program entry point. e.g: 'entry 0x401000'" << endl;
    paramsList << Logger::getLogger().getLogColorStr(LogLevel::WARNING, true) + "   bp" + Logger::getLogger().getLogColorStr(LogLevel::INFO) + " -> Specifies a program breakpoint and the length of instructions to be replaced. e.g: 'bp 0x401000:20'" << endl;
    paramsList << Logger::getLogger().getLogColorStr(LogLevel::WARNING, true) + "   args" + Logger::getLogger().getLogColorStr(LogLevel::INFO) + " -> Specifies the arguments to be passed to the traced binary. e.g: 'args \"./name.bin hello world\"'" << endl;
    paramsList << Logger::getLogger().getLogColorStr(LogLevel::WARNING, true) + "   script" + Logger::getLogger().getLogColorStr(LogLevel::INFO) + " -> Provides a script to automatically execute the debugger. e.g: 'script \"./auto.hscript\"'" << endl;
    paramsList << Logger::getLogger().getLogColorStr(LogLevel::WARNING, true) + "   output" + Logger::getLogger().getLogColorStr(LogLevel::INFO) + " -> Redirects the formatted output of the tool into a file. e.g: 'output \"./output.hout\"'" << endl;
    paramsList << Logger::getLogger().getLogColorStr(LogLevel::WARNING, true) + "   config" + Logger::getLogger().getLogColorStr(LogLevel::INFO) + " -> Provides command parameters from a file. e.g: 'config \"./config.hconf\"'" << endl;
    Logger::getLogger().log(LogLevel::INFO, paramsList.str(), false, false);
    Logger::getLogger().log(LogLevel::INFO, "Flags:", false, false);
    stringstream flagsList;
    flagsList << Logger::getLogger().getLogColorStr(LogLevel::WARNING, true) + "   help" + Logger::getLogger().getLogColorStr(LogLevel::INFO) + " -> Displays this message.";
    Logger::getLogger().log(LogLevel::INFO, flagsList.str(), false, false);
}

bool addBreakpoint(const string& val) {
    int delimiterIndex = (int)val.find(':');
    if(delimiterIndex == string::npos) {
        stringstream msg;
        msg << "Breakpoints need to follow this format -> " + Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) + "address:length" + Logger::getLogger().getLogColorStr(LogLevel::FATAL) + " !";
        Logger::getLogger().log(LogLevel::FATAL, msg.str());
        return false;
    }
    string addrPart = val.substr(0, delimiterIndex);
    string lenPart = val.substr(delimiterIndex + 1);
    BigInt addr = 0, lenBuf = 0;
    unsigned char len;
    if(!inputToNumber(addrPart, addr)) {
        stringstream msg;
        msg << "Specified breakpoint address " + Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << addrPart << Logger::getLogger().getLogColorStr(LogLevel::FATAL) + " is not a number !";
        Logger::getLogger().log(LogLevel::FATAL, msg.str());
        return false;
    }
    if(!inputToNumber(lenPart, lenBuf)) {
        stringstream msg;
        msg << "Specified breakpoint length " + Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << lenPart << Logger::getLogger().getLogColorStr(LogLevel::FATAL) + " is not a number !";
        Logger::getLogger().log(LogLevel::FATAL, msg.str());
        return false;
    }
    if(lenBuf > 64) {
        stringstream msg;
        msg << "Specified breakpoint length " + Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << lenPart << Logger::getLogger().getLogColorStr(LogLevel::FATAL) + " is greater than 64 !";
        Logger::getLogger().log(LogLevel::FATAL, msg.str());
        return false;
#if __x86_64__
    } else if(lenBuf < 12) {
        stringstream msg;
        msg << "Specified breakpoint length " + Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << lenPart << Logger::getLogger().getLogColorStr(LogLevel::FATAL) + " is smaller than 12 !";
        Logger::getLogger().log(LogLevel::FATAL, msg.str());
        return false;
    }
#else
    } else if(lenBuf < 8) {
        stringstream msg;
        msg << "Specified breakpoint length " + Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << lenPart << Logger::getLogger().getLogColorStr(LogLevel::FATAL) + " is smaller than 8 !";
        Logger::getLogger().log(LogLevel::FATAL, msg.str());
        return false;
    }
#endif
    len = lenBuf;
    params.breakpoints[addr] = len;
    params.bpIndexFromAddr[addr] = bpIndex;
    bpIndex++;
    return true;
}

bool analyseParam(const string& param, const string& val) {
    if(!param.find("--")) {
        string paramName = param.substr(2);
        if(paramName == "help") printHelpMessage();
    } else if(!param.find('-')) {
        string paramName = param.substr(1);
        if(!val.length()) {
            stringstream msg;
            msg << "Parameter " + Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << paramName << Logger::getLogger().getLogColorStr(LogLevel::FATAL) + " requires a value !";
            Logger::getLogger().log(LogLevel::FATAL, msg.str());
            return false;
        }
        if(paramName == "entry") {
            BigInt entryAddress = 0;
            if(inputToNumber(val, entryAddress)) {
                params.entryAddress = entryAddress;
            } else {
                stringstream msg;
                msg << "Specified entry address " + Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << val << Logger::getLogger().getLogColorStr(LogLevel::FATAL) + " is not a number !";
                Logger::getLogger().log(LogLevel::FATAL, msg.str());
                return false;
            }
        } else if(paramName == "bp") {
            if(!addBreakpoint(val)) return false;
        } else if(paramName == "args") {
            vector<string> args;
            split(val, ' ', args);
            params.binaryArgs = args;
        } else if(paramName == "script") {
            string path = filesystem::current_path().u8string() + "/" + val;
            params.scriptFile = new ifstream();
            params.scriptFile->open(path);
            if (params.scriptFile->is_open()) {
                Logger::getLogger().log(LogLevel::SUCCESS, "Script file found !");
            } else {
                stringstream msg;
                msg << "Couldn't open script file " + Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << path << Logger::getLogger().getLogColorStr(LogLevel::FATAL) + " !";
                Logger::getLogger().log(LogLevel::FATAL, msg.str());
            }
        } else if(paramName == "output") {
            string path = filesystem::current_path().u8string() + "/" + val;
            params.outputFile = new ofstream();
            params.outputFile->open(path);
            if (params.outputFile->is_open()) {
                Logger::getLogger().log(LogLevel::SUCCESS, "Output file found !");
            } else {
                stringstream msg;
                msg << "Couldn't open output file " + Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << path << Logger::getLogger().getLogColorStr(LogLevel::FATAL) + " !";
                Logger::getLogger().log(LogLevel::FATAL, msg.str());
            }
        } else if(paramName == "config") {
            string path = filesystem::current_path().u8string() + "/" + val;
            ifstream configFile = ifstream();
            configFile.open(path);
            if (configFile.is_open()) {
                Logger::getLogger().log(LogLevel::SUCCESS, "Config file found ! Reading it...");
                string line;
                while (getline(configFile, line)) {
                    if (!line.rfind("entry ", 0)) {
                        string entryAddr = line.substr(string("entry ").size());
                        if(!inputToNumber(entryAddr, params.entryAddress)) {
                            stringstream msg;
                            msg << "Entry " + Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << entryAddr << Logger::getLogger().getLogColorStr(LogLevel::FATAL) + " is not a valid integer !";
                            Logger::getLogger().log(LogLevel::FATAL, msg.str());
                            return false;
                        }
                    } else if(!line.rfind("bp ", 0)) {
                        string bpAddrStr = line.substr(string("bp ").size());
                        if(!addBreakpoint(bpAddrStr)) return false;
                    }
                }
            } else {
                stringstream msg;
                msg << "Couldn't open config file " + Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << path << Logger::getLogger().getLogColorStr(LogLevel::FATAL) + " !";
                Logger::getLogger().log(LogLevel::FATAL, msg.str());
            }
        }
    }
    return true;
}

bool checkSudo() {
    uid_t effectiveUID = geteuid();
    return !effectiveUID;
}

void handleExit() {
    if(dbg != nullptr) dbg->handleExit();
}

void sigHandler(int) {
    handleExit();
    exit(1);
}

void prepareSigHandler() {
    struct sigaction sigIntHandler = {};
    sigIntHandler.sa_handler = sigHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, nullptr);
}

int main(int argc, char* argv[]) {
    prepareSigHandler();
    ConfigFileManager::getInstance();
    if(argc > 1) {
        if(!strcmp(argv[1], "--help")) {
            printHelpMessage();
            return 0;
        }
        char* binaryName = argv[1];
        string binaryPath = (binaryName[0] == '/' ? "" : filesystem::current_path().u8string()) + "/" + binaryName;
        if(filesystem::exists(binaryPath)) {
            params.binaryPath = binaryPath;
            for (int i = 2; i < argc; i++) {
                if(!analyseParam(argv[i], i+1 < argc ? argv[i+1] : "")) return 0;
            }
            if(params.binaryArgs.empty()) params.binaryArgs.push_back(params.binaryPath);
            if(!checkSudo()) Logger::getLogger().log(LogLevel::WARNING, "Running the tool without root rights (sudo) may cause issues.");
            dbg = new HadesDbg(params);
            dbg->run();
        } else {
            stringstream msg;
            msg << "File " + Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << binaryPath << Logger::getLogger().getLogColorStr(LogLevel::FATAL) + " does not exist !";
            Logger::getLogger().log(LogLevel::FATAL, msg.str());
        }
    } else {
        printHelpMessage();
    }
    return 0;
}