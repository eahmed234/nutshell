#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct CMD {
    std::string command;
    std::vector<std::string> args;

    void addCommand(std::string _command) {
        command = _command;
    }

    void addArg(std::string arg) {
        args.push_back(arg);
    }
};

extern CMD currCommand;

extern std::unordered_map<std::string, std::string> aliases;

extern std::unordered_map<std::string, std::string> envs;

extern std::vector<std::string> reserved;

void parseCMD();