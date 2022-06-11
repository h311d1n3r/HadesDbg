
#ifndef HADESDBG_LOG_H
#define HADESDBG_LOG_H

#include <string>
#include <sstream>
#include <fstream>

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
    ofstream* outputFile;
public:
    void log(LogLevel level, const string& message, bool bold, bool prefixSymbol);
    void log(LogLevel level, const string& message, bool bold) {log(level, message, bold, true);};
    void log(LogLevel level, const string& message) {log(level, message, false, true);};
    void setOutputFile(ofstream* outputFile);
    static Logger getLogger();
};

#endif