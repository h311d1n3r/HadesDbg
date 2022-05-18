#ifndef HADESDBG_UTILS_H
#define HADESDBG_UTILS_H

#include <string>
#include <vector>
using namespace std;

void split(const string &str, char c, vector<string> &elements);
bool inputToNumber(string input, unsigned long long int& number);
string toUppercase(string input);
bool endsWith(std::string const &value, std::string const &ending);
string executeCommand(const char* cmd);
unsigned long long int invertEndian(unsigned long long int val);

#endif
