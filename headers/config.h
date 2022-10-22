#ifndef HADESDBG_CONFIG_H
#define HADESDBG_CONFIG_H

#include <utils.h>

struct Config {
    long openDelayMilli;
};

class ConfigFileManager {
public:
    ConfigFileManager();
    static ConfigFileManager* getInstance();
    Config* getConfig();
private:
    Config* readConfig();
    void writeConfig(Config config);
    bool configExists();
    Config* config;
    static ConfigFileManager* instance;
};

#endif //HADESDBG_CONFIG_H
