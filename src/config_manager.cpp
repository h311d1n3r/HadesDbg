#include <config.h>
#include <json.h>
#include <filesystem>
#include <log.h>

string configPath = findHomeDir()+"/.hadesdbg/";
string configName = "config.json";

Config defaultConfig = {
    200
};

ConfigFileManager* ConfigFileManager::instance = nullptr;

ConfigFileManager::ConfigFileManager() {
    if(!this->configExists()) {
        this->writeConfig(defaultConfig);
    }
    this->config = readConfig();
}

Config* ConfigFileManager::readConfig() {
    Config* conf = (Config*) malloc(sizeof(Config));
    if(conf) {
        ifstream configFile(configPath + configName);
        if(configFile.is_open()) {
            stringstream jsonStr;
            jsonStr << configFile.rdbuf();
            json::jobject configJson = json::jobject::parse(jsonStr.str());
            conf->openDelayMilli = configJson["open_delay_milli"];
            configFile.close();
            return conf;
        } else {
            stringstream errorStream;
            errorStream << "Couldn't open config file \033[;37m" << configPath << configName << "\033[;35m ! Missing rights ?";
            Logger::getLogger().log(LogLevel::ERROR, errorStream.str());
            Logger::getLogger().log(LogLevel::WARNING, "Using default config file.");
        }
    }
    return &defaultConfig;
}

void ConfigFileManager::writeConfig(Config config) {
    json::jobject configJson;
    configJson["open_delay_milli"] = config.openDelayMilli;
    string jsonStr = configJson.pretty();
    if(!filesystem::exists(configPath)) filesystem::create_directories(configPath);
    ofstream configFile(configPath + configName);
    if(configFile.is_open()) {
        configFile << jsonStr;
        configFile.close();
    } else {
        stringstream errorStream;
        errorStream << "Couldn't open config file \033[;37m" << configPath << configName << "\033[;35m ! Missing rights ?" << endl;
        Logger::getLogger().log(LogLevel::ERROR, errorStream.str());
    }
}

bool ConfigFileManager::configExists() {
    return filesystem::exists(configPath + configName);
}

Config* ConfigFileManager::getConfig() {
    return this->config;
}

ConfigFileManager* ConfigFileManager::getInstance() {
    if(!instance) {
        instance = new ConfigFileManager();
    }
    return instance;
}