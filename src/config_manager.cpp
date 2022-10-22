#include <config.h>
#include <json.h>
#include <filesystem>
#include <log.h>

string configPath = findHomeDir()+"/.hadesdbg/";
string configName = "config.json";

Config defaultConfig = {
    200,
    Theme::CETUS
};

ConfigFileManager* ConfigFileManager::instance = nullptr;

ConfigFileManager::ConfigFileManager() {
    if(!this->configExists()) {
        this->config = &defaultConfig;
    } else this->config = readConfig();
    this->writeConfig(*this->config);
}

Config* ConfigFileManager::readConfig() {
    Config* conf = (Config*) malloc(sizeof(Config));
    if(conf) {
        ifstream configFile(configPath + configName);
        if(configFile.is_open()) {
            stringstream jsonStr;
            jsonStr << configFile.rdbuf();
            json::jobject configJson = json::jobject::parse(jsonStr.str());
            if(configJson.has_key("open_delay_milli")) conf->openDelayMilli = configJson["open_delay_milli"];
            else conf->openDelayMilli = defaultConfig.openDelayMilli;
            if(configJson.has_key("theme")) conf->theme = stringToTheme(configJson["theme"]);
            else conf->theme = defaultConfig.theme;
            configFile.close();
            return conf;
        } else {
            stringstream errorStream;
            errorStream << "Couldn't open config file " << Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << configPath << configName << Logger::getLogger().getLogColorStr(LogLevel::ERROR) << " ! Missing rights ?";
            Logger::getLogger().log(LogLevel::ERROR, errorStream.str());
            Logger::getLogger().log(LogLevel::WARNING, "Using default config file.");
        }
    }
    return &defaultConfig;
}

Theme ConfigFileManager::stringToTheme(std::string themeStr) {
    Theme theme = defaultConfig.theme;
    if(toUppercase(themeStr) == "CETUS") {
        theme = Theme::CETUS;
    } else if(toUppercase(themeStr) == "CERBERUS") {
        theme = Theme::CERBERUS;
    } else if(toUppercase(themeStr) == "BASILISK") {
        theme = Theme::BASILISK;
    }
    return theme;
}

string ConfigFileManager::themeToString(Theme theme) {
    string themeStr;
    switch(theme) {
        case CETUS:
            themeStr = "cetus";
            break;
        case CERBERUS:
            themeStr = "cerberus";
            break;
        case BASILISK:
            themeStr = "basilisk";
            break;
        default:
            this->themeToString(defaultConfig.theme);
    }
    return themeStr;
}

void ConfigFileManager::writeConfig(Config config) {
    json::jobject configJson;
    configJson["open_delay_milli"] = config.openDelayMilli;
    configJson["theme"] = this->themeToString(config.theme);
    string jsonStr = configJson.pretty();
    if(!filesystem::exists(configPath)) filesystem::create_directories(configPath);
    ofstream configFile(configPath + configName);
    if(configFile.is_open()) {
        configFile << jsonStr;
        configFile.close();
    } else {
        stringstream errorStream;
        errorStream << "Couldn't open config file " << Logger::getLogger().getLogColorStr(LogLevel::VARIABLE) << configPath << configName << Logger::getLogger().getLogColorStr(LogLevel::ERROR) << " ! Missing rights ?" << endl;
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