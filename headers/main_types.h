#ifndef HADESDBG_MAIN_TYPES_H
#define HADESDBG_MAIN_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <fstream>

using namespace std;

#if __x86_64__
#define BigInt unsigned long long int
#else
#define BigInt unsigned int
#endif

struct BinaryParams {
    string binaryPath;
    vector<string> binaryArgs;
    BigInt entryAddress;
    map<BigInt, unsigned char> breakpoints;
    map<BigInt, unsigned int> bpIndexFromAddr;
    ifstream* scriptFile;
    ofstream* outputFile;
};

#endif