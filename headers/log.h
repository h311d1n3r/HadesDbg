
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
    BLACK [[maybe_unused]] = 30,
    RED [[maybe_unused]] = 31,
    GREEN [[maybe_unused]] = 32,
    YELLOW [[maybe_unused]] = 33,
    BLUE [[maybe_unused]] = 34,
    MAGENTA [[maybe_unused]] = 35,
    CYAN [[maybe_unused]] = 36,
    WHITE [[maybe_unused]] = 37,
    BRIGHT_BLACK [[maybe_unused]] = 90,
    BRIGHT_RED [[maybe_unused]] = 91,
    BRIGHT_GREEN [[maybe_unused]] = 92,
    BRIGHT_YELLOW [[maybe_unused]] = 93,
    BRIGHT_BLUE [[maybe_unused]] = 94,
    BRIGHT_MAGENTA [[maybe_unused]] = 95,
    BRIGHT_CYAN [[maybe_unused]] = 96,
    BRIGHT_WHITE [[maybe_unused]] = 97
};

class Logger {
private:
    static Logger* instance;
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