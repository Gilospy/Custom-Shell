#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <ncurses.h>

#include "token.h"
#include "command.h"

#define MAX_LINE 1024 // maximum length of input line
#define MAX_NUM_COMMANDS 100
#define PATH_LENGTH 1024
#define MAX_HISTORY 100



void addHistory(const char *line , char *history[], int* historyCount) {
    if (*historyCount < MAX_HISTORY) {
        history[*historyCount] = strdup(line);
        (*historyCount)++;
    } else {
        // Shift left and replace oldest command
        free(history[0]);
        memmove(history, history + 1, (MAX_HISTORY-1) * sizeof(char*));
        history[MAX_HISTORY-1] = strdup(line);
    }
}

int handleHistoryExpansion(char *line, char *expandedLine , char *history[], int historyCount) {
    // !! -> repeat last command
    if (strcmp(line, "!!") == 0) {
        if (historyCount == 0) {
            printf("No commands in history.\n");
            return -1;
        }
        strcpy(expandedLine, history[historyCount-1]);
        return 0;
    }

    // !<number> -> repeat specific command
    if (line[1] >= '0' && line[1] <= '9') {
        int cmdNum = atoi(line + 1);
        if (cmdNum < 1 || cmdNum > historyCount) {
            printf("No such command in history.\n");
            return -1;
        }
        strcpy(expandedLine, history[cmdNum - 1]);
        return 0;
    }

    // !<string> -> repeat last command starting with string
    if (line[1] != '\0') { // not just "!"
        char *prefix = line + 1; // skip '!'
        for (int i = historyCount-1; i >= 0; i--) {
            if (strncmp(history[i], prefix, strlen(prefix)) == 0) {
                strcpy(expandedLine, history[i]);
                return 0;
            }
        }
        printf("No command starts with '%s'.\n", prefix);
        return -1;
    }

    return -1; // Not handled
}



int collectChildren() {
    int status;
    pid_t pid;

    while (status) {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0) {
            status = 0; // No more zombies
        }
    }
    return 0;
}

void handleSignals(int signo) {
    if (signo == SIGCHLD) {
        collectChildren();
    }
}

int setupSignalHandlers() {
    struct sigaction sa;
    
    sa.sa_handler = handleSignals; // Function pointer to handler
    sa.sa_flags = 0;

    if(sigaction(SIGCHLD, &sa, NULL) != 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    
    sigset_t signal_set;
    sigemptyset(&signal_set); // Initialize empty signal set
    sigaddset(&signal_set, SIGINT); // Add SIGINT to the set (Ctrl+C)
    sigaddset(&signal_set, SIGQUIT); // Add SIGQUIT to the set (Ctrl+\)
    sigaddset(&signal_set, SIGTSTP); // Add SIGTSTOP to the set (Ctrl+Z)

    // Block the signals in the set
    if (sigprocmask(SIG_BLOCK, &signal_set, NULL) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    return 0;
}


int toggleSigChild(int enable) {
    sigset_t signal_set;
    sigemptyset(&signal_set); // Initialize empty signal set
    sigaddset(&signal_set, SIGCHLD); // Add SIGCHLD to the set

    if (enable) {
        // Unblock SIGCHLD
        if (sigprocmask(SIG_UNBLOCK, &signal_set, NULL) < 0) {
            perror("sigprocmask unblock");
            exit(EXIT_FAILURE);
        }
    } else {
        // Block SIGCHLD
        if (sigprocmask(SIG_BLOCK, &signal_set, NULL) < 0) {
            perror("sigprocmask block");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

int executeCommandLine(char *line , char* prompt , char *history[] , int* historyCount) {
    char *tokens[MAX_NUM_TOKENS];
    Command commands[MAX_NUM_COMMANDS];
    int tokenCount, cmdCount;

    // Add to history
    addHistory(line, history, historyCount);

    // Tokenize input line
    tokenCount = tokenise(line, tokens);
    if (tokenCount == -1) {
        printf("Too many tokens!\n");
        return -1;
    }

    // Separate commands
    cmdCount = separateCommands(tokens, commands);

    if (cmdCount == -1) {
        printf("Syntax error: command cannot start with a separator.\n");
        return -1;
    }
    else if (cmdCount == -2) {
        printf("Syntax error: Consecutive Separators.\n");
        return -1;
    }
    else if (cmdCount == -3) {
        printf("Syntax error: last command cannot end with '|'.\n");
        return -1;
    }
    else if (cmdCount == -4) {
        printf("Error: Too many arguments in a command.\n");
        return -1;
    }

    // Block SIGCHLD before executing commands
    toggleSigChild(0);

    // Execute commands
    executeCommands(commands, cmdCount , prompt , history , *historyCount);

    // Unblock SIGCHLD after executing commands
    toggleSigChild(1);

    return 0;
}

int main() {
    char line[MAX_LINE];
    char prompt[64] = "\%";
    char *history[MAX_HISTORY];
    int historyCount = 0;

    // Setup signal handlers
    setupSignalHandlers();

    while (1) {
        printf("%s ", prompt);
        if (!fgets(line, sizeof(line), stdin)) break;

        if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';

        // Check if line is empty
        if (strlen(line) == 0) continue;
        
        if (line[0] == '!') {
            char expandedLine[MAX_LINE];
            if (handleHistoryExpansion(line, expandedLine , history , historyCount) == -1) {
                continue; // skip if error
            }
            strcpy(line, expandedLine);
            printf("%s\n", line); // optional: show the expanded command
        }

        executeCommandLine(line , prompt , history , &historyCount);
   
    }

    return 0;
}