#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include<sys/wait.h>  
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

void execCMD() {
    string binPath = "/bin/" + currCommand.command;
    vector<char*> args = { &binPath[0] };
    for (auto& arg : currCommand.args) {
        args.push_back(&arg[0]);
    }
    pid_t p = fork();
    if (p == 0) {
        execv(binPath.c_str(), &args[0]); 
    } else {
        wait(NULL);
    }
}

void parseCMD() {
    if (find(reserved.begin(), reserved.end(), currCommand.command) == reserved.end()) {
        execCMD();
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
    } else if (currCommand.command == "unsetenv") {
        envs.erase(currCommand.args.at(0));
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
    envs["PATH"] = "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin";

    while (true) {
        cout << "> " << flush;
        yyparse();
    }
    return 0;
}