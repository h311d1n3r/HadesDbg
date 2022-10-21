#ifndef HADESDBG_UTILS_H
#define HADESDBG_UTILS_H

#include <string>
#include <vector>
#include <main_types.h>

using namespace std;

void split(const string &str, char c, vector<string> &elements);
bool inputToNumber(string input, BigInt& number);
string toUppercase(string input);
bool endsWith(std::string const &value, std::string const &ending);
string executeCommand(const char* cmd);
BigInt invertEndian(BigInt val);
string removeConsoleChars(string consoleOut);

#endif
