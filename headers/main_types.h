#ifndef HADESDBG_MAIN_TYPES_H
#define HADESDBG_MAIN_TYPES_H

#include <string>
#include <vector>
#include <map>

using namespace std;

struct BinaryParams {
    string binaryPath;
    vector<string> binaryArgs;
    unsigned long long int entryAddress;
    map<unsigned long long int, unsigned char> breakpoints;
};

#endif