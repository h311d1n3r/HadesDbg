#include <config.h>

string configPath = findHomeDir()+"/.hadesdbg/";
string configName = "config.json";

ConfigFileManager* ConfigFileManager::instance = nullptr;

ConfigFileManager::ConfigFileManager() {
    
}

ConfigFileManager* ConfigFileManager::getInstance() {
    if(!instance) {
        instance = new ConfigFileManager();
    }
    return instance;
}