#include <iostream>
#include <log.h>

using namespace std;

Logger* Logger::instance;

Logger Logger::getLogger() {
    if(!Logger::instance) {
        Logger::instance = new Logger();
    }
    return *Logger::instance;
}

void Logger::log(LogLevel level, const string& message, bool bold, bool prefixSymbol) {
    stringstream prefix;
    prefix << "\033[" << (bold?"1":"0") << ";";
    switch(level) {
        case LogLevel::SUCCESS:
            prefix << "32m" << (prefixSymbol ? "[+] " : "");
            break;
        case LogLevel::INFO:
            prefix << "36m" << (prefixSymbol ? "[*] " : "");
            break;
        case LogLevel::WARNING:
            prefix << "33m" << (prefixSymbol ? "[#] " : "");
            break;
        case LogLevel::ERROR:
            prefix << "35m" << (prefixSymbol ? "[?] " : "");
            if(this->outputFile && this->outputFile->is_open()) *this->outputFile << "Error" << endl;
            break;
        case LogLevel::FATAL:
            prefix << "31m" << (prefixSymbol ? "[!] " : "");
            if(this->outputFile && this->outputFile->is_open()) *this->outputFile << "Fatal error" << endl;
            break;
    }
    prefix << message << "\033[0m";
    cout << prefix.str() << endl;
}

void Logger::setOutputFile(ofstream* outputFile) {
    this->outputFile = outputFile;
}