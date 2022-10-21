#ifndef HADESDBG_CONFIG_H
#define HADESDBG_CONFIG_H

#include <utils.h>

struct Config {
    long openDelayMilli = 0;
};

class ConfigFileManager {
public:
    ConfigFileManager();
    static ConfigFileManager* getInstance();
private:
    static ConfigFileManager* instance;
};

#endif //HADESDBG_CONFIG_H
