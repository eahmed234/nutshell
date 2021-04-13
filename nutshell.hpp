#pragma once
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>

struct Line;

extern Line line;

extern std::map<std::string, std::string> aliases;

extern std::map<std::string, std::string> envs;

std::string expandVars(std::string s);

void parseLine();

struct Line {
    struct CMD {
        std::string command;
        std::vector<std::string> args;
    };

    bool stderrRedirect = false;
    bool stderrToFile;
    std::string stderrRedirectFile;

    bool inputRedirect = false;
    std::string input;

    bool outputRedirect = false;
    bool append;
    std::string output;

    int i = -1;
    std::vector<CMD> commands;

    std::string expandAlias(std::string s) {
        auto it = aliases.find(s);
        if (it != aliases.end()) {
            std::string fullCommand = it->second;
            fullCommand.erase(remove(fullCommand.begin(), fullCommand.end(), '\"'), fullCommand.end());
            std::stringstream ss(fullCommand);
            std::string command;
            ss >> command;
            std::string arg;
            while (ss >> arg) {
                addArg(arg);
            }
            return command;
        }
        return s;
    }

    void addCommand(std::string _command) {
        ++i;
        CMD c;
        commands.push_back(c);
        commands.at(i).command = expandAlias(_command);
    }

    void addArg(std::string _arg) {
        commands.at(i).args.push_back(_arg);
    }

    void reset() {
        inputRedirect = false;
        outputRedirect = false;
        stderrRedirect = false;
        i = -1;
        commands.clear();
        input.clear();
        output.clear();
        stderrRedirectFile.clear();
    }
};