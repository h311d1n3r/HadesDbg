#include <utils.h>
#include <regex>
#include <log.h>
#include <unistd.h>
#include <pwd.h>

void split(const string &str, char c, vector<string> &elements) {
    stringstream strStream(str);
    string subStr;
    while (getline(strStream, subStr, c)) {
        elements.push_back(subStr);
    }
}

bool inputToNumber(string input, BigInt& number) {
    size_t inputEnd = 0;
    int base = 10;
    if(input.find("0x") == 0) {
        base = 16;
        input = input.substr(2);
    } else if(input.find('a') != string::npos || input.find('b') != string::npos || input.find('c') != string::npos
              || input.find('d') != string::npos || input.find('e') != string::npos || input.find('f') != string::npos) base = 16;
    regex validNumber("[0-9a-f]+");
    if(regex_match(input,validNumber)) number = stoull(input, &inputEnd, base);
    return inputEnd;
}

string toUppercase(string input) {
    for (char& c: input) c = (char)toupper(c);
    return input;
}

bool endsWith(std::string const &value, std::string const &ending) {
    if(ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "LocalValueEscapesScope"
string executeCommand(const char* cmd) {
    array<char, 128> buffer = {};
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        Logger::getLogger().log(LogLevel::ERROR, "Unable to execute command !");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}
#pragma clang diagnostic pop

BigInt invertEndian(BigInt val) {
    unsigned long long int ret = 0;
    char* valArr = (char*)&val;
    char* retArr = (char*)&ret;
    for(int i = 0; i < sizeof(val); i++) {
        retArr[i] = valArr[sizeof(val)-1-i];
    }
    return ret;
}

string removeConsoleChars(string consoleOut) {
    string ret;
    for(int i = 0; i < consoleOut.length(); i++) {
        char currentChar = consoleOut[i];
        if(currentChar == '\033') {
            i += 6;
        } else ret += currentChar;
    }
    return ret;
}

string findHomeDir() {
    string homeDir;
    if(!(homeDir = getenv("HOME")).length()) {
        homeDir = getpwuid(getuid())->pw_dir;
    }
    return homeDir;
}

string trim(string str) {
    unsigned int index;
    string output;
    while((index = str.find_first_of(' ')) != string::npos) {
        output += str.substr(0,index);
        str = str.substr(index+1);
    }
    return output+str;
}