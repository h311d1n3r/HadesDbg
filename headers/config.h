#ifndef HADESDBG_CONFIG_H
#define HADESDBG_CONFIG_H

#include <utils.h>

enum Theme {
    CETUS,
    CERBERUS,
    BASILISK
};

struct Config {
    long openDelayMilli;
    Theme theme;
};

class ConfigFileManager {
public:
    ConfigFileManager();
    static ConfigFileManager* getInstance();
    Config* getConfig();
private:
    Config* readConfig();
    void writeConfig(Config config);
    Theme stringToTheme(string themeStr);
    string themeToString(Theme theme);
    bool configExists();
    Config* config;
    static ConfigFileManager* instance;
};

#endif //HADESDBG_CONFIG_H
