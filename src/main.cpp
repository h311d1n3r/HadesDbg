#include <iostream>
#include <string>
#include <log.h>
#include <signal.h>
#include <unistd.h>
#include <filesystem>
#include <string.h>
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

void printHelpMessage() {
    Logger::getLogger().log(LogLevel::INFO, TOOL_TITLE, false, true, false);
    stringstream version;
    version << " Version: " << VERSION;
    Logger::getLogger().log(LogLevel::INFO, version.str(), false, true, false);
    stringstream author;
    author << " Author: " << AUTHOR;
    Logger::getLogger().log(LogLevel::INFO, author.str(), false, false, false);
    Logger::getLogger().log(LogLevel::INFO, "________________________________________\n", false, true, false);
    Logger::getLogger().log(LogLevel::INFO, "Syntax: \033[1;33mhadesdbg binary [-param value] [--flag]\n", false, true, false);
    Logger::getLogger().log(LogLevel::INFO, "Parameters:", false, true, false);
    stringstream paramsList;
    paramsList << "\033[1;33m   entry\033[0;36m -> Specifies the program entry point. e.g: 'entry 0x401000'" << endl;
    paramsList << "\033[1;33m   args\033[0;36m -> Specifies the arguments to be passed to the traced binary. e.g: 'args \"./name.bin hello world\"'" << endl;
    Logger::getLogger().log(LogLevel::INFO, paramsList.str(), false, true, false);
    Logger::getLogger().log(LogLevel::INFO, "Flags:", false, true, false);
    stringstream flagsList;
    flagsList << "\033[1;33m   help\033[0;36m -> Displays this message." << endl;
    Logger::getLogger().log(LogLevel::INFO, flagsList.str(), false, true, false);
}

bool analyseParam(string param, string val) {
    if(!param.find("--")) {
        string paramName = param.substr(2);
        if(!paramName.compare("help")) printHelpMessage();
    } else if(!param.find("-")) {
        string paramName = param.substr(1);
        if(!val.length()) {
            stringstream msg;
            msg << "Parameter \033[;37m" << paramName << "\033[;31m requires a value !";
            Logger::getLogger().log(LogLevel::FATAL, msg.str());
            return false;
        }
        if(!paramName.compare("entry")) {
            unsigned long long int entryAddress = 0;
            if(inputToNumber(val, entryAddress)) {
                params.entryAddress = entryAddress;
            } else {
                stringstream msg;
                msg << "Specified entry address \033[;37m" << val << "\033[;31m is not a number !";
                Logger::getLogger().log(LogLevel::FATAL, msg.str());
                return false;
            }
        } else if(!paramName.compare("args")) {
            vector<string> args;
            split(val, ' ', args);
            params.binaryArgs = args;
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

void sigHandler(int signal) {
    handleExit();
    exit(1);
}

void prepareSigHandler() {
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sigHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
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
            if(!params.binaryArgs.size()) params.binaryArgs.push_back(params.binaryPath);
            if(!checkSudo()) Logger::getLogger().log(LogLevel::WARNING, "Running the tool without root rights (sudo) may cause issues.");
            dbg = new HadesDbg(params);
            dbg->run();
        } else {
            stringstream msg;
            msg << "File \033[;37m" << binaryPath << "\033[;31m does not exist !";
            Logger::getLogger().log(LogLevel::FATAL, msg.str());
        }
    } else {
        Logger::getLogger().log(LogLevel::FATAL, "You need to specify more parameters -> \033[;37mhadesdbg binary");
    }
    return 0;
}