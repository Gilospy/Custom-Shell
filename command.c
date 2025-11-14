// file:		command.c for Week 8
// purpose;		to separate a list of tokens into a sequence of commands.
// assumptions:		any two successive commands in the list of tokens are separated 
//			by one of the following command separators:
//				"|"  - pipe to the next command
//				"&"  - shell does not wait for the proceeding command to terminate
//				";"  - shell waits for the proceeding command to terminate
// author:		Gilchrist
// date:		18/10/2025



#include <string.h> 
#include <stdlib.h>
#include <glob.h>
#include <wordexp.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <ncurses.h>


#include "command.h"

void expandString(const char *input, char *output, size_t outSize) {
    if (input == NULL || output == NULL) return;

    // Use wordexp for tilde (~) and env variable ($VAR) expansion
    wordexp_t p;
    if (wordexp(input, &p, 0) == 0 && p.we_wordc > 0) {
        // Copy the expanded word to output
        snprintf(output, outSize, "%s", p.we_wordv[0]);
        wordfree(&p);
    } else {
        // If expansion fails, just copy input as-is
        snprintf(output, outSize, "%s", input);
    }
}

int isSeparator(const char *token) {
    return token && (strcmp(token, pipeSep) == 0 ||
                     strcmp(token, conSep)  == 0 ||
                     strcmp(token, seqSep)  == 0);
}
// Utility: check if a string contains a wildcard
int has_wildcard(const char *s) {
    return (strchr(s, '*') != NULL || strchr(s, '?') != NULL);    
}

int separateCommands(char *tokens[], Command command[]) {
    int cmdCount = 0;
    int i = 0;

    // Check if first token is a separator (error condition)
    if (!tokens[0] || isSeparator(tokens[0])) {
        return -1;
    }

    while (tokens[i]) { // While there are tokens left / NULL not reached
        // Initialize the command structure
        command[cmdCount].argc = 0;
        command[cmdCount].separator = NULL;
        command[cmdCount].redirectTo = NULL;
        command[cmdCount].redirectType = 0;
        
        // First token of the command is the command name
        command[cmdCount].command = tokens[i];
        command[cmdCount].argv[command[cmdCount].argc] = tokens[i];
        command[cmdCount].argc++;
        i++;

        // Collect arguments until we hit a separator or end
        // Handle redirection tokens here 
        while (tokens[i] && !isSeparator(tokens[i])) {
            if (command[cmdCount].argc >= MAX_NUM_TOKENS) {
                return -4; // Too many arguments
            }
            // Handle input redirection
            if (strcmp(tokens[i], inputSep) == 0) {
                i++;
                if (tokens[i]) {
                    command[cmdCount].redirectTo = tokens[i];
                    command[cmdCount].redirectType = 1; // output redirection
                    i++;
                    continue;
                }
            }
            // Handle output redirection
            if (strcmp(tokens[i], outputSep) == 0) {
                i++;
                if (tokens[i]) {
                    command[cmdCount].redirectTo = tokens[i];
                    command[cmdCount].redirectType = 2; // output redirection
                    i++;
                    continue;
                }
            }
            // Handle error redirection
            if (strcmp(tokens[i], errorSep) == 0) {
                i++;
                if (tokens[i]) {
                    command[cmdCount].redirectTo = tokens[i];
                    command[cmdCount].redirectType = 3; // error redirection
                    i++;
                    continue;
                }
            }
            
            char expandedArg[1024];
            expandString(tokens[i], expandedArg, sizeof(expandedArg));
            command[cmdCount].argv[command[cmdCount].argc] = expandedArg;
            command[cmdCount].argc++;
            i++;
        }
        
        // NULL-terminate the argv array
        command[cmdCount].argv[command[cmdCount].argc] = NULL;

        // Assign separator if exists
        if (tokens[i]) {
            command[cmdCount].separator = tokens[i];
            i++; // skip separator
            
            // Check for consecutive separators
            if (tokens[i] && isSeparator(tokens[i])) {
                return -2; // Consecutive separators
            }
        }

        cmdCount++;
    }

    // Error: cannot end with pipe separator
    if (cmdCount > 0) {
        const char *lastSep = command[cmdCount - 1].separator;
        if (lastSep && strcmp(lastSep, pipeSep) == 0) {
            return -3; // Last command cannot end with '|'
        }
    }

    return cmdCount;
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
    int in_fd = 0; // Input for first command (stdin)
    pid_t pids[cmdCount];  

    for (int i = 0; i < cmdCount; i++) {
        if (i < cmdCount - 1) {
            // Create a pipe for all but the last command
            if (pipe(pipefd) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } 
        else if (pids[i] == 0) {
            // Child process

            // If not the first command, set input from previous pipe
            if (in_fd != 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            // If not the last command, set output to current pipe
            if (i < cmdCount - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]); // Close unused read end
                close(pipefd[1]); // Close duplicated write end
            }

            // Handle any redirection
            setIO(cmds[i]);

            // Execute the command
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
        }
    }

    // Wait for all children after forking them
    for (int i = 0; i < cmdCount; i++) {
        waitpid(pids[i], NULL, 0);
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
