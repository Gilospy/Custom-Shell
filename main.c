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

void initInput() {
    initscr();            // Start ncurses mode
    cbreak();             // Disable line buffering
    noecho();             // Don't echo characters automatically
    keypad(stdscr, TRUE); // Enable arrow keys
}



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



int setIO(Command cmd) {
    char *filename; 
    int fd = cmd.redirectType;
    int file;

    if(fd == 0) {
        return 0; // No redirection
    }
    else if(fd == 1) {
        // Input redirection
        filename = cmd.redirectTo;
        file = open(filename, O_RDONLY);        
    }
    else if(fd == 2) {
        // Output redirection
        filename = cmd.redirectTo;
        file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);        
    }
    else if(fd == 3) {
        // Error redirection
        filename = cmd.redirectTo;
        file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);        
    }

    if(file < 0) {
        perror("Couldnt open file");
        exit(EXIT_FAILURE);
    }
    else{
        if(fd == 1) {
            dup2(file, STDIN_FILENO);
        } else if(fd == 2) {
            dup2(file, STDOUT_FILENO);
        } else if(fd == 3) {
            dup2(file, STDERR_FILENO);
        }
        close(file);
        return 0;
    }
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

int handleBuiltIn(Command* cmd , char* prompt , char *history[] , int historyCount) {
    if (strcmp(cmd->command, CD_CMD) == 0) {
        const char* path;
        if (cmd->argc < 2) {
            path = getenv("HOME");
        } else {
            path = cmd->argv[1];
        }
        // Cannot handle $HOME or ~ expansion here yet
        if (chdir(path) != 0) {
            fprintf(stderr, "cd: %s: %s\n", path, strerror(errno));
        }
        return 1;
    } 
    else if (strcmp(cmd->command, PROMPT_CMD) == 0) {
        if (cmd->argc < 2) {
            strncpy(prompt, "\\%", 63);
        } else {
            strncpy(prompt, cmd->argv[1], 63);
            prompt[63] = '\0'; // Ensure null-termination
        }
        return 1;
    } 
    else if (strcmp(cmd->command, PWD_CMD) == 0) {
        char cwd[PATH_LENGTH];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
        return 1;
    }
    else if (strcmp(cmd->command, HISTORY_CMD) == 0) {        
        for (int i = 0; i < historyCount; i++) {
            printf("%d %s\n", i+1, history[i]);
        }    
        return 1;
    }
    else if (strcmp(cmd->command, EXIT_CMD) == 0) {
        exit(EXIT_SUCCESS);
    }
    return 0; // Not a built-in command
}

void executeCommand(Command cmd) {
   
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } 
    else if (pid == 0) {
        // Child process
        setIO(cmd);
        if (execvp(cmd.command, cmd.argv) < 0) {
            fprintf(stderr, "%s: command not found\n", cmd.command);
            exit(EXIT_FAILURE);
        }
    } 
    else {
        // Parent process
        if (cmd.separator == NULL || strcmp(cmd.separator, seqSep) == 0) {
            // Wait for child to finish if sequential execution
            waitpid(pid, NULL, 0); // Dont care about exit status here
        }
    }
}

void executePipeCommands(Command* cmds, int cmdCount) {
    int pipefd[2];
    pid_t pid;
    int in_fd = 0; // Initial input is from stdin

    for (int i = 0; i < cmdCount; i++) {
        if (i < cmdCount - 1) {
            // Create a pipe for all but the last command
            if (pipe(pipefd) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } 
        else if (pid == 0) {
            // Child process
            if (in_fd != 0) {
                // If not first command, set input from previous pipe
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < cmdCount - 1) {
                // If not last command, set output to current pipe
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]); // Close unused read end
                close(pipefd[1]); // Close write end after duplicating
            }
            setIO(cmds[i]); // Handle redirection if any
            if (execvp(cmds[i].command, cmds[i].argv) < 0) {
                fprintf(stderr, "%s: command not found\n", cmds[i].command);
                exit(EXIT_FAILURE);
            }
        } 
        else {
            // Parent process
            if (in_fd != 0) {
                close(in_fd); // Close previous read end
            }
            if (i < cmdCount - 1) {
                close(pipefd[1]); // Close write end of current pipe
                in_fd = pipefd[0]; // Next command reads from here
            }
            else {
                // On the last command
                close(pipefd[0]); // Close read end of last pipe
            }
            waitpid(pid, NULL, 0); // Wait for child to finish
        }
    }
}

void executeCommands(Command* cmds, int cmdCount , char* prompt , char *history[] , int historyCount) {
    int pipeCount = 0;
    Command pipeCommands[MAX_NUM_COMMANDS];
    int i = 0;
    while (i < cmdCount) {
        if(handleBuiltIn(&cmds[i], prompt, history, historyCount)) {
           i++;
           continue;
        }

        // Print command arguements for debugging
        // printf("Executing command: %s\n", cmds[i].command);
        // for(int j = 0; j < cmds[i].argc; j++) {
        //     printf("Arg %d: %s\n", j, cmds[i].argv[j]);
        // }

        // For pipe commands
        if (cmds[i].separator != NULL && strcmp(cmds[i].separator, pipeSep) == 0) {
            pipeCommands[pipeCount] = cmds[i];
            pipeCount++;
            i++;            
        }else{
            if (pipeCount > 0) {
                // There are pipe commands to execute
                pipeCommands[pipeCount] = cmds[i]; // Add last command in the pipe
                pipeCount++;
                executePipeCommands(pipeCommands, pipeCount);
                pipeCount = 0; // Reset for next set of pipe commands
            } else {
                // Single command execution
                executeCommand(cmds[i]);
            }
            i++;
        }       
    }
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

int getInputWithHistory(char *line, char **history, int historyCount, const char *prompt) {
    int pos = 0;
    int ch;
    int currentHistory = historyCount; // index for navigating history

    memset(line, 0, MAX_LINE);
    printw("%s", prompt);
    refresh();

    while ((ch = getch()) != '\n') {
        if (ch == KEY_BACKSPACE || ch == 127) {
            if (pos > 0) {
                pos--;
                line[pos] = '\0';
                int y, x;
                getyx(stdscr, y, x);
                move(y, x-1);
                delch();
                refresh();
            }
        } else if (ch == KEY_UP) {
            if (historyCount == 0) continue;
            if (currentHistory > 0) currentHistory--;
            move(getcury(stdscr), strlen(prompt));
            clrtoeol();
            strcpy(line, history[currentHistory]);
            printw("%s", line);
            pos = strlen(line);
            refresh();
        } else if (ch == KEY_DOWN) {
            if (historyCount == 0) continue;
            if (currentHistory < historyCount - 1) {
                currentHistory++;
                move(getcury(stdscr), strlen(prompt));
                clrtoeol();
                strcpy(line, history[currentHistory]);
                printw("%s", line);
                pos = strlen(line);
            } else {
                // beyond last history: clear line
                currentHistory = historyCount;
                move(getcury(stdscr), strlen(prompt));
                clrtoeol();
                line[0] = '\0';
                pos = 0;
            }
            refresh();
        } else {
            if (pos < MAX_LINE - 1) {
                line[pos++] = ch;
                addch(ch);
                refresh();
            }
        }
    }

    line[pos] = '\0';
    printw("\n");
    refresh();
    return pos;
}


int main() {
    char line[MAX_LINE];
    char prompt[64] = "\%";
    char *history[MAX_HISTORY];
    int historyCount = 0;

    // Setup signal handlers
    setupSignalHandlers();
    initInput(); // Initialize ncurses for input handling

    while (1) {
        printw("%s ", prompt);
        
        refresh();

        if (getInputWithHistory(line, history, historyCount, prompt) == 0) continue;


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
    endwin(); // End ncurses mode
    return 0;
}