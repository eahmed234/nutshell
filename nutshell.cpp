#include <iostream>
#include <algorithm>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/wait.h>  
#include "nutshell.tab.hpp"
using namespace std;

CMD currCommand;

map<string, string> aliases;

map<string, string> envs;

vector<string> reserved = {
    "setenv",
    "printenv",
    "unsetenv",
    "cd",
    "alias",
    "unalias",
    "bye"
};

vector<string> pathVars;

void updatePathVars() {
    pathVars.clear();
    stringstream ss(envs["PATH"]);
    string item;

    while (getline(ss, item, ':')) {
        pathVars.push_back (item);
    }
}

int execCMD(string binPath) {
    vector<char*> args = { &binPath[0] };
    for (auto& arg : currCommand.args) {
        args.push_back(&arg[0]);
    }
    args.push_back(NULL);
    pid_t p = fork();
    if (p == 0) {
        execv(binPath.c_str(), &args[0]);
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(p, &status, 0);
        return WIFEXITED(status);
    }
    return 0;
}

void execHelper() {
    if (currCommand.command[0] == '.' || currCommand.command[0] == '/') {
        execCMD(currCommand.command);
    }
    for (auto& path : pathVars) {
        if (execCMD(path + '/' + currCommand.command) == 0)
            return;
    }
}

void parseCMD() {
    if (find(reserved.begin(), reserved.end(), currCommand.command) == reserved.end()) {
        execHelper();
        currCommand.command.clear();
        currCommand.args.clear();
        return;
    }
    if (currCommand.command == "alias") {
        if (currCommand.args.empty()) {
            for (auto& alias : aliases) {
                cout << alias.first << " " << alias.second << '\n';
            }
        } else {
            aliases[currCommand.args.at(0)] = currCommand.args.at(1);
        }
    } else if (currCommand.command == "unalias") {
        aliases.erase(currCommand.args.at(0));
    } else if (currCommand.command == "printenv") {
        for(auto& env : envs) {
            cout << env.first << "=" << env.second << '\n';
        }
    } else if (currCommand.command == "setenv") {
        envs[currCommand.args.at(0)] = currCommand.args.at(1);
        updatePathVars();
    } else if (currCommand.command == "unsetenv") {
        envs.erase(currCommand.args.at(0));
    } else if (currCommand.command == "cd") {
        chdir(currCommand.args.at(0).c_str());
    }
    currCommand.command.clear();
    currCommand.args.clear();
}

int main() {
    const char* homedir;
    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    envs["HOME"] = homedir;
    envs["PATH"] = "/home/eahmed234/lab5:/bin:/usr/local/bin";
    updatePathVars();

    while (true) {
        cout << "> " << flush;
        yyparse();
    }
    return 0;
}