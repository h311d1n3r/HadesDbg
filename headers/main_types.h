#ifndef HADESDBG_MAIN_TYPES_H
#define HADESDBG_MAIN_TYPES_H

#include <string>
#include <vector>

using namespace std;

struct BinaryParams {
    string binaryPath;
    vector<string> binaryArgs;
    unsigned long long int entryAddress;
};

#endif