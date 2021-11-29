
#ifndef HADESDBG_LOG_H
#define HADESDBG_LOG_H

#include <string>
#include <sstream>

using namespace std;

enum LogLevel {
    SUCCESS,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
private:
    static Logger* instance;
public:
    void log(LogLevel level, string message, bool bold, bool endLine, bool prefixSymbol);
    void log(LogLevel level, string message, bool bold, bool endLine) {log(level, message, bold, endLine, true);};
    void log(LogLevel level, string message, bool bold) {log(level, message, bold, true, true);};
    void log(LogLevel level, string message) {log(level, message, false, true, true);};
    static Logger getLogger();
};

#endif