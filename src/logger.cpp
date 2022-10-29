#include <iostream>
#include <log.h>

using namespace std;

Logger* Logger::instance;

Logger::Logger() {
    Config* config = ConfigFileManager::getInstance()->getConfig();
    this->initTheme(config->theme);
}

void Logger::initTheme(Theme theme) {
    switch(theme) {
        case Theme::CETUS:
            this->colorsByLevels[LogLevel::VARIABLE] = LogColor::WHITE;
            this->colorsByLevels[LogLevel::SUCCESS] = LogColor::GREEN;
            this->colorsByLevels[LogLevel::INFO] = LogColor::CYAN;
            this->colorsByLevels[LogLevel::WARNING] = LogColor::YELLOW;
            this->colorsByLevels[LogLevel::ERROR] = LogColor::MAGENTA;
            this->colorsByLevels[LogLevel::FATAL] = LogColor::RED;
            break;
        case Theme::CERBERUS:
            this->colorsByLevels[LogLevel::VARIABLE] = LogColor::RED;
            this->colorsByLevels[LogLevel::SUCCESS] = LogColor::BRIGHT_YELLOW;
            this->colorsByLevels[LogLevel::INFO] = LogColor::BRIGHT_RED;
            this->colorsByLevels[LogLevel::WARNING] = LogColor::YELLOW;
            this->colorsByLevels[LogLevel::ERROR] = LogColor::CYAN;
            this->colorsByLevels[LogLevel::FATAL] = LogColor::BLUE;
            break;
        case Theme::BASILISK:
            this->colorsByLevels[LogLevel::VARIABLE] = LogColor::GREEN;
            this->colorsByLevels[LogLevel::SUCCESS] = LogColor::BRIGHT_YELLOW;
            this->colorsByLevels[LogLevel::INFO] = LogColor::BRIGHT_GREEN;
            this->colorsByLevels[LogLevel::WARNING] = LogColor::BLUE;
            this->colorsByLevels[LogLevel::ERROR] = LogColor::BRIGHT_RED;
            this->colorsByLevels[LogLevel::FATAL] = LogColor::RED;
            break;
    }
}

Logger Logger::getLogger() {
    if(!Logger::instance) {
        Logger::instance = new Logger();
    }
    return *Logger::instance;
}

void Logger::log(LogLevel level, const string& message, bool bold, bool prefixSymbol) {
    stringstream prefix;
    prefix << "\033[" << (bold?"1":"0") << ";" << +colorsByLevels[level] << "m";
    switch(level) {
        case LogLevel::SUCCESS:
            prefix << (prefixSymbol ? "[+] " : "");
            break;
        case LogLevel::INFO:
            prefix << (prefixSymbol ? "[*] " : "");
            break;
        case LogLevel::WARNING:
            prefix << (prefixSymbol ? "[#] " : "");
            break;
        case LogLevel::ERROR:
            prefix << (prefixSymbol ? "[?] " : "");
            break;
        case LogLevel::FATAL:
            prefix << (prefixSymbol ? "[!] " : "");
            break;
        case VARIABLE:
            break;
    }
    prefix << message << "\033[0m";
    cout << prefix.str() << endl;
}

string Logger::getLogColorStr(LogLevel level, bool bold) {
    LogColor color = LogColor::WHITE;
    if(colorsByLevels.count(level)) color = colorsByLevels[level];
    stringstream colorStream;
    colorStream << "\033[" << (bold?"1":"0") << ";" << +color << "m";
    return colorStream.str();
}