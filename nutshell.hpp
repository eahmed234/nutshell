#pragma once
#include <string>
#include <vector>
#include <map>

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

extern std::map<std::string, std::string> aliases;

extern std::map<std::string, std::string> envs;

void parseCMD();