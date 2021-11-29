#include <utils.h>
#include <regex>

void split(const string &str, char c, vector<string> &elements) {
    stringstream strStream(str);
    string subStr;
    while (getline(strStream, subStr, c)) {
        elements.push_back(subStr);
    }
}

string trim(string str) {
    int index;
    string output;
    while((index = str.find_first_of(" ")) != string::npos) {
        output += str.substr(0,index);
        str = str.substr(index+1);
    }
    return output+str;
}

bool inputToNumber(string input, unsigned long long int& number) {
    size_t inputEnd = 0;
    int base = 10;
    if(input.find("0x") == 0) {
        base = 16;
        input = input.substr(2);
    } else if(input.find("a") != string::npos || input.find("b") != string::npos || input.find("c") != string::npos
              || input.find("d") != string::npos || input.find("e") != string::npos || input.find("f") != string::npos) base = 16;
    regex validNumber("[0-9a-f]+");
    if(regex_match(input,validNumber)) number = stoull(input, &inputEnd, base);
    return inputEnd;
}

string toUppercase(string input) {
    string upperCase = input;
    for (char& c: upperCase) c = toupper(c);
    return upperCase;
}