#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include "nutshell.tab.hpp"
using namespace std;

CMD currCommand;

unordered_map<string, string> aliases;

unordered_map<string, string> envs;

void parseCMD() {
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
    currCommand.command = "";
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