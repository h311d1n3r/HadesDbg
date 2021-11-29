#include <iostream>
#include <log.h>

using namespace std;

Logger* Logger::instance;

Logger Logger::getLogger() {
    if(!Logger::instance) {
        *Logger::instance = Logger();
    }
    return *Logger::instance;
}

void Logger::log(LogLevel level, string message, bool bold, bool endLine, bool prefixSymbol) {
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
            break;
        case LogLevel::FATAL:
            prefix << "31m" << (prefixSymbol ? "[!] " : "");
            break;
    }
    prefix << message << "\033[0m";
    cout << prefix.str() << endl;
}