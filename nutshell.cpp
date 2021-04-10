#include <iostream>
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/wait.h> 
#include <limits.h> 
#include "nutshell.tab.hpp"
#define READ_END 0
#define WRITE_END 1
using namespace std;

Line line;

map<string, string> aliases;

map<string, string> envs;

vector<string> pathVars;

void updatePathVars() {
    pathVars.clear();
    stringstream ss(envs["PATH"]);
    string item;

    while (getline(ss, item, ':')) {
        pathVars.push_back(item);
    }
}

int execCMD(string binPath) {
    vector<char*> args = { &binPath[0] };
    for (auto& arg : line.commands.at(0).args) {
        args.push_back(&arg[0]);
    }
    args.push_back(NULL);
    pid_t p = fork();

    if (p < 0) {
        cerr << "Fork failed\n";
        return -1; 
    } else if (p == 0) {
        execv(binPath.c_str(), &args[0]);
        exit(1);
    } else {
        int status;
        wait(&status);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
    }
    return 0;
}

void execHelper() {
    bool succ = false;
    if (line.commands.at(0).command[0] == '/') {
        if (execCMD(line.commands.at(0).command) == 0) succ = true;
    } else {
        for (auto& path : pathVars) {
            if (execCMD(path + '/' + line.commands.at(0).command) == 0) {
                succ = true;
                break;
            }
        }
    }
    if (!succ)
        cerr << "Invalid command\n";
}

void execSingleCMD() {
    Line::CMD currCommand = line.commands.at(0);

    auto it = aliases.find(currCommand.command);
    if (it != aliases.end()) {
        string fullCommand = it->second;
        fullCommand.erase(remove(fullCommand.begin(), fullCommand.end(), '\"'), fullCommand.end());
        stringstream ss(fullCommand);
        string command;
        ss >> command;
        currCommand.command = command;
        currCommand.args.clear();
        string arg;
        while (ss >> arg) {
            currCommand.args.push_back(arg);
        }
        execSingleCMD();
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
        if (currCommand.args.size() == 0) {
            chdir(envs["HOME"].c_str());
        } else {
            chdir(currCommand.args.at(0).c_str());
        }
    } else {
        execHelper();
    }
}

void execMultiCMD() {
    vector<char*> args1 = { &line.commands.at(0).command[0] };
    for (auto& arg : line.commands.at(0).args) {
        args1.push_back(&arg[0]);
    }
    args1.push_back(NULL);

    vector<char*> args2 = { &line.commands.at(1).command[0] };
    for (auto& arg : line.commands.at(1).args) {
        args2.push_back(&arg[0]);
    }
    args2.push_back(NULL);

    pid_t pid;
    int fd[2];

    pipe(fd);
    pid = fork();

    if(pid==0) {
        dup2(fd[WRITE_END], STDOUT_FILENO);
        close(fd[READ_END]);
        close(fd[WRITE_END]);
        execvp(line.commands.at(0).command.c_str(), &args1[0]);
        cerr << "Failed to execute" << endl;
        exit(1);
    } else { 
        pid=fork();

        if(pid==0) {
            dup2(fd[READ_END], STDIN_FILENO);
            close(fd[WRITE_END]);
            close(fd[READ_END]);
            execvp(line.commands.at(1).command.c_str(), &args2[0]);
            cerr << "Failed to execute" << endl;
            exit(1);
        } else {
            int status;
            close(fd[READ_END]);
            close(fd[WRITE_END]);
            waitpid(pid, &status, 0);
        }
    }
}

void parseLine() {
    if (line.commands.size() == 1) {
        execSingleCMD();
    } else {
        execMultiCMD();
    }
    line.reset();
} 

string expandVars(string s) {
    if (s.find("~") != string::npos) {
        size_t pos = 0;
        while ((pos = s.find("~", pos)) != string::npos) {
            s.replace(pos, 1, envs["HOME"]);
            pos += envs["HOME"].length();
        }
    }

    if( s.find( "${" ) == string::npos ) return s;

    string pre  = s.substr( 0, s.find( "${" ) );
    string post = s.substr( s.find( "${" ) + 2 );

    if( post.find( '}' ) == string::npos ) return s;

    string variable = post.substr( 0, post.find( '}' ) );
    string value = envs[variable];

    post = post.substr( post.find( '}' ) + 1 );

    return expandVars( pre + value + post );
}

int main() {
    auto pw = getpwuid(getuid());
    const char* homedir;
    if ((homedir = getenv("HOME")) == NULL) {
        homedir = pw->pw_dir;
    }
    envs["HOME"] = homedir;

    const char* path;
    if ((path = getenv("PATH")) == NULL) {
        path = "/bin:.";
    }
    envs["PATH"] = path;
    updatePathVars();

    string username = pw->pw_name;
    string currPath;

    while (true) {
        currPath = filesystem::current_path();
        cout << username << ":" << currPath << "> " << flush;
        yyparse();
    }
    return 0;
}