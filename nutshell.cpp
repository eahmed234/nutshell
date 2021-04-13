#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/wait.h> 
#include <sys/stat.h>
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
        cerr << "Fork failed" << endl;
        return -1; 
    } else if (p == 0) {
        if (line.stderrRedirect) {
            int redirect;
            if (line.stderrToFile) {
                redirect = open(line.stderrRedirectFile.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
            } else {
                redirect = STDOUT_FILENO;
            }
            close(STDERR_FILENO);
            dup2(redirect, STDERR_FILENO);
        }
        if (line.outputRedirect) {
            int redirect;
            if (line.append) {
                redirect = open(line.output.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
            } else {
                redirect = open(line.output.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
            }
            close(STDOUT_FILENO);
            dup2(redirect, STDOUT_FILENO);
        }
        if (line.inputRedirect) {
            int redirect = open(line.input.c_str(), O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR);
            close(STDIN_FILENO);
            dup2(redirect, STDIN_FILENO);
        }
        execv(binPath.c_str(), &args[0]);
        exit(1);
    } else {
        int status;
        wait(&status);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
    }
    return 0;
}

void execMultiCMD() {
    int fd[2];
	pid_t pid;
	int fdd = 0;
    string binPath;

	for (size_t i = 0; i < line.commands.size(); ++i) {
        if (line.commands.at(i).command.at(0) == '/') {
            struct stat sb;
            if (stat(line.commands.at(i).command.c_str(), &sb) == 0 && sb.st_mode & S_IXUSR) {
                binPath = line.commands.at(i).command;
            } else {
                cerr << "Invalid Command" << endl;
                return;
            }
        } else {
            bool succ = false;
            for (auto& path : pathVars) {
                binPath = path + '/' + line.commands.at(i).command;
                struct stat sb;
                if (stat(binPath.c_str(), &sb) == 0 && sb.st_mode & S_IXUSR) {
                    succ = true;
                    break;
                }
            }
            if (!succ) {
                cerr << "Invalid Command" << endl;
                return; 
            }
        }

        vector<char*> args = { &line.commands.at(i).command[0] };
        for (auto& arg : line.commands.at(i).args) {
            args.push_back(&arg[0]);
        }
        args.push_back(NULL);

		pipe(fd);
		if ((pid = fork()) == -1) {
			cerr << "Fork failed" << endl;
			exit(1);
		} else if (pid == 0) {
            if (line.stderrRedirect) {
                int redirect;
                if (line.stderrToFile) {
                    redirect = open(line.stderrRedirectFile.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
                } else {
                   redirect = STDOUT_FILENO;
                }
                close(STDERR_FILENO);
                dup2(redirect, STDERR_FILENO);
            }
            if (line.inputRedirect && i == 0) {
                int redirect = open(line.input.c_str(), O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR);
                close(STDIN_FILENO);
                dup2(redirect, STDIN_FILENO);
            } else {
			    dup2(fdd, STDIN_FILENO);
            }
			if (i < line.commands.size() - 1) {
				dup2(fd[WRITE_END], STDOUT_FILENO);
			} else if (line.outputRedirect && i == line.commands.size() - 1) {
                int redirect;
                if (line.append) {
                    redirect = open(line.output.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
                } else {
                    redirect = open(line.output.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
                }
                close(STDOUT_FILENO);
                dup2(redirect, STDOUT_FILENO);
            }
			close(fd[READ_END]);
			execv(binPath.c_str(), &args[0]);
			exit(1);
		} else {
			wait(NULL);
			close(fd[WRITE_END]);
			fdd = fd[READ_END];
		}
	}
}

void execHelper() {
    if (line.commands.size() > 1) {
        execMultiCMD();
        return;
    }
    bool succ = false;
    if (line.commands.at(0).command[0] == '/') {
        if (execCMD(line.commands.at(0).command) == 0) succ = true;
    } else {
        string binPath = "";
        for (auto& path : pathVars) {
            binPath = path + '/' + line.commands.at(0).command;
            struct stat sb;
            if (stat(binPath.c_str(), &sb) == 0 && sb.st_mode & S_IXUSR) break;
        }
        if (execCMD(binPath) == 0) succ = true;
    }
    if (!succ)
        cerr << "Invalid command\n";
}

void parseLine() {
    Line::CMD currCommand = line.commands.at(0);

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

    line.reset();
} 

int main() {
    auto pw = getpwuid(getuid());
    const char* homedir;
    if ((homedir = getenv("HOME")) == NULL) {
        homedir = pw->pw_dir;
    }
    envs["HOME"] = homedir;

    string path;
    path = getenv("PATH");
    path = ".:/bin:" + path;
    envs["PATH"] = path;
    updatePathVars();

    char* username = pw->pw_name;
    char currPath[PATH_MAX];

    while (true) {
        getcwd(currPath, PATH_MAX);
        cout << username << ":" << currPath << "> " << flush;
        yyparse();
    }
    return 0;
}