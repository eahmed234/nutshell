#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include<sys/wait.h>  
#include "nutshell.tab.hpp"
using namespace std;

CMD currCommand;

unordered_map<string, string> aliases;

unordered_map<string, string> envs;

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
    pid_t p = fork();
    if (p == 0) {
        switch (currCommand.args.size()) {
            case 0:
                execl(binPath.c_str(), binPath.c_str(), NULL);
                break;
            case 1:
                execl(binPath.c_str(), binPath.c_str(), currCommand.args.at(0).c_str(), NULL);
                break;
        }  
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

    while (true) {
        cout << "> " << flush;
        yyparse();
    }
    return 0;
}