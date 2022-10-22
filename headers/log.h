
#ifndef HADESDBG_LOG_H
#define HADESDBG_LOG_H

#include <string>
#include <sstream>
#include <map>
#include <config.h>

using namespace std;

enum LogLevel {
    VARIABLE,
    SUCCESS,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

enum LogColor {
    BLACK = 30,
    RED = 31,
    GREEN = 32,
    YELLOW = 33,
    BLUE = 34,
    MAGENTA = 35,
    CYAN = 36,
    WHITE = 37,
    BRIGHT_BLACK = 90,
    BRIGHT_RED = 91,
    BRIGHT_GREEN = 92,
    BRIGHT_YELLOW = 93,
    BRIGHT_BLUE = 94,
    BRIGHT_MAGENTA = 95,
    BRIGHT_CYAN = 96,
    BRIGHT_WHITE = 97
};

class Logger {
private:
    static Logger* instance;
    ofstream* outputFile;
    map<LogLevel, LogColor> colorsByLevels;
    void initTheme(Theme theme);
public:
    Logger();
    void log(LogLevel level, const string& message, bool bold, bool prefixSymbol);
    void log(LogLevel level, const string& message, bool bold) {log(level, message, bold, true);};
    void log(LogLevel level, const string& message) {log(level, message, false, true);};
    string getLogColorStr(LogLevel level, bool bold=false);
    static Logger getLogger();
};

#endif