#include <regs.h>

map<string, Register> registerFromName = {
        {"RSP", Register::RSP},
        {"RAX", Register::RAX},
        {"RBX", Register::RBX},
        {"RCX", Register::RCX},
        {"RDX", Register::RDX},
        {"RDI", Register::RDI},
        {"RSI", Register::RSI},
        {"R8", Register::R8},
        {"R9", Register::R9},
        {"R10", Register::R10},
        {"R11", Register::R11},
        {"R12", Register::R12},
        {"R13", Register::R13},
        {"R14", Register::R14},
        {"R15", Register::R15},
        {"RBP", Register::RBP},
        {"RIP", Register::RIP}
};

vector<string> orderedRegsNames = {
        "RSP",
        "RAX",
        "RBX",
        "RCX",
        "RDX",
        "RDI",
        "RSI",
        "R8",
        "R9",
        "R10",
        "R11",
        "R12",
        "R13",
        "R14",
        "R15",
        "RBP",
        "RIP"
};