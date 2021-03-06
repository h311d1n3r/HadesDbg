#include <iostream>
#include <string>
#include <log.h>
#include <csignal>
#include <unistd.h>
#include <filesystem>
#include <cstring>
#include <vector>
#include <main_types.h>
#include <utils.h>
#include <debugger.h>

using namespace std;

const string TOOL_TITLE = "  _  _         _        ___  _          \n"
                          " | || |__ _ __| |___ __|   \\| |__  __ _ \n"
                          " | __ / _` / _` / -_|_-< |) | '_ \\/ _` |\n"
                          " |_||_\\__,_\\__,_\\___/__/___/|_.__/\\__, |\n"
                          "                                  |___/ ";
const string VERSION = "1.0";
const string AUTHOR = "h311d1n3r";

BinaryParams params;
HadesDbg* dbg;
unsigned int bpIndex = 1;

void printHelpMessage() {
    Logger::getLogger().log(LogLevel::INFO, TOOL_TITLE, false, false);
    stringstream version;
    version << " Version: " << VERSION;
    Logger::getLogger().log(LogLevel::INFO, version.str(), false, false);
    stringstream author;
    author << " Author: " << AUTHOR;
    Logger::getLogger().log(LogLevel::INFO, author.str(), false, false);
    Logger::getLogger().log(LogLevel::INFO, "________________________________________\n", false, false);
    Logger::getLogger().log(LogLevel::INFO, "Syntax: \033[1;33mhadesdbg binary [-param value] [--flag]\n", false, false);
    Logger::getLogger().log(LogLevel::INFO, "Parameters:", false, false);
    stringstream paramsList;
    paramsList << "\033[1;33m   entry\033[0;36m -> Specifies the program entry point. e.g: 'entry 0x401000'" << endl;
    paramsList << "\033[1;33m   bp\033[0;36m -> Specifies a program breakpoint and the length of instructions to be replaced. e.g: 'bp 0x401000:20'" << endl;
    paramsList << "\033[1;33m   args\033[0;36m -> Specifies the arguments to be passed to the traced binary. e.g: 'args \"./name.bin hello world\"'" << endl;
    paramsList << "\033[1;33m   script\033[0;36m -> Provides a script to automatically execute the debugger. e.g: 'script \"./auto.hscript\"'" << endl;
    paramsList << "\033[1;33m   output\033[0;36m -> Redirects the formatted output of the tool into a file. e.g: 'output \"./output.hout\"'" << endl;
    paramsList << "\033[1;33m   config\033[0;36m -> Provides command parameters from a file. e.g: 'config \"./config.hconf\"'" << endl;
    Logger::getLogger().log(LogLevel::INFO, paramsList.str(), false, false);
    Logger::getLogger().log(LogLevel::INFO, "Flags:", false, false);
    stringstream flagsList;
    flagsList << "\033[1;33m   help\033[0;36m -> Displays this message." << endl;
    Logger::getLogger().log(LogLevel::INFO, flagsList.str(), false, false);
}

bool addBreakpoint(string val) {
    int delimiterIndex = (int)val.find(':');
    if(delimiterIndex == string::npos) {
        stringstream msg;
        msg << "Breakpoints need to follow this format -> \033[;37maddress:length\033[;31m !";
        Logger::getLogger().log(LogLevel::FATAL, msg.str());
        return false;
    }
    string addrPart = val.substr(0, delimiterIndex);
    string lenPart = val.substr(delimiterIndex + 1);
    unsigned long long int addr = 0, lenBuf = 0;
    unsigned char len;
    if(!inputToNumber(addrPart, addr)) {
        stringstream msg;
        msg << "Specified breakpoint address \033[;37m" << addrPart << "\033[;31m is not a number !";
        Logger::getLogger().log(LogLevel::FATAL, msg.str());
        return false;
    }
    if(!inputToNumber(lenPart, lenBuf)) {
        stringstream msg;
        msg << "Specified breakpoint length \033[;37m" << lenPart << "\033[;31m is not a number !";
        Logger::getLogger().log(LogLevel::FATAL, msg.str());
        return false;
    }
    if(lenBuf > 64) {
        stringstream msg;
        msg << "Specified breakpoint length \033[;37m" << lenPart << "\033[;31m is greater than 64 !";
        Logger::getLogger().log(LogLevel::FATAL, msg.str());
        return false;
    } else if(lenBuf < 12) {
        stringstream msg;
        msg << "Specified breakpoint length \033[;37m" << lenPart << "\033[;31m is smaller than 12 !";
        Logger::getLogger().log(LogLevel::FATAL, msg.str());
        return false;
    }
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
            msg << "Parameter \033[;37m" << paramName << "\033[;31m requires a value !";
            Logger::getLogger().log(LogLevel::FATAL, msg.str());
            return false;
        }
        if(paramName == "entry") {
            unsigned long long int entryAddress = 0;
            if(inputToNumber(val, entryAddress)) {
                params.entryAddress = entryAddress;
            } else {
                stringstream msg;
                msg << "Specified entry address \033[;37m" << val << "\033[;31m is not a number !";
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
                msg << "Couldn't open script file \033[;37m" << path << "\033[;31m !";
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
                msg << "Couldn't open output file \033[;37m" << path << "\033[;31m !";
                Logger::getLogger().log(LogLevel::FATAL, msg.str());
            }
        } else if(paramName == "config") {
            string path = filesystem::current_path().u8string() + "/" + val;
            ifstream configFile = ifstream();
            configFile.open(path);
            if (configFile.is_open()) {
                Logger::getLogger().log(LogLevel::SUCCESS, "Config file found ! Reading it...");
                string line;
                unsigned int bpIndex = 0;
                while (getline(configFile, line)) {
                    if (!line.rfind("entry ", 0)) {
                        string entryAddr = line.substr(string("entry ").size());
                        if(!inputToNumber(entryAddr, params.entryAddress)) {
                            stringstream msg;
                            msg << "Entry \033[;37m" << entryAddr << "\033[;31m is not a valid integer !";
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
                msg << "Couldn't open config file \033[;37m" << path << "\033[;31m !";
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
            msg << "File \033[;37m" << binaryPath << "\033[;31m does not exist !";
            Logger::getLogger().log(LogLevel::FATAL, msg.str());
        }
    } else {
        printHelpMessage();
    }
    return 0;
}