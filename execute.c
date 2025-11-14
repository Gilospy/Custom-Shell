#include "shell.h"

/* Execute array of commands */
void executeCommands(struct Command_struct commands[], int num_commands) {
    int i = 0;
    
    while (i < num_commands) {
        /* Find the end of the current job (pipeline) */
        int job_start = i;
        int job_count = 1;
        
        while (i < num_commands && commands[i].com_suffix == '|') {
            i++;
            job_count++;
        }
        i++;  /* Move past the last command in the job */
        
        /* Check if this is a built-in command */
        if (job_count == 1 && isBuiltIn(commands[job_start].com_pathname)) {
            executeBuiltIn(&commands[job_start]);
            continue;
        }
        
        /* Determine if this is a background job */
        int is_background = (commands[job_start + job_count - 1].com_suffix == '&');
        
        /* Execute the job (single command or pipeline) */
        if (job_count == 1) {
            executeSingleCommand(&commands[job_start]);
        } else {
            executePipeline(commands, job_start, job_count);
        }
        
        /* Wait for foreground jobs */
        if (!is_background) {
            int status;
            while (wait(&status) > 0);
        }
    }
}

/* Execute a single command */
void executeSingleCommand(struct Command_struct *cmd) {
    pid_t pid;
    int status;
    
    /* Expand wildcards */
    expandWildcards(cmd);
    
    pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return;
    }
    
    if (pid == 0) {
        /* Child process */
        
        /* Handle input redirection */
        if (cmd->redirect_in != NULL) {
            int fd = open(cmd->redirect_in, O_RDONLY);
            if (fd < 0) {
                perror(cmd->redirect_in);
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        
        /* Handle output redirection */
        if (cmd->redirect_out != NULL) {
            int fd = open(cmd->redirect_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror(cmd->redirect_out);
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        
        /* Handle error redirection */
        if (cmd->redirect_err != NULL) {
            int fd = open(cmd->redirect_err, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror(cmd->redirect_err);
                exit(1);
            }
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        
        /* Execute the command */
        execvp(cmd->com_pathname, cmd->argv);
        
        /* If execvp returns, there was an error */
        fprintf(stderr, "%s: command not found\n", cmd->com_pathname);
        exit(127);
    }
    
    /* Parent process */
    if (cmd->com_suffix != '&') {
        /* Foreground job - wait for completion */
        waitpid(pid, &status, 0);
    }
}

/* Execute a pipeline of commands */
void executePipeline(struct Command_struct commands[], int start, int count) {
    int i;
    int pipes[MAX_COMMANDS - 1][2];
    pid_t pids[MAX_COMMANDS];
    int status;
    
    /* Create all pipes */
    for (i = 0; i < count - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }
    }
    
    /* Fork and execute each command in the pipeline */
    for (i = 0; i < count; i++) {
        struct Command_struct *cmd = &commands[start + i];
        
        /* Expand wildcards */
        expandWildcards(cmd);
        
        pids[i] = fork();
        
        if (pids[i] < 0) {
            perror("fork");
            return;
        }
        
        if (pids[i] == 0) {
            /* Child process */
            
            /* Set up input */
            if (i > 0) {
                /* Not the first command - read from previous pipe */
                dup2(pipes[i - 1][0], STDIN_FILENO);
            } else if (cmd->redirect_in != NULL) {
                /* First command with input redirection */
                int fd = open(cmd->redirect_in, O_RDONLY);
                if (fd < 0) {
                    perror(cmd->redirect_in);
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            
            /* Set up output */
            if (i < count - 1) {
                /* Not the last command - write to next pipe */
                dup2(pipes[i][1], STDOUT_FILENO);
            } else if (cmd->redirect_out != NULL) {
                /* Last command with output redirection */
                int fd = open(cmd->redirect_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror(cmd->redirect_out);
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            
            /* Handle error redirection */
            if (cmd->redirect_err != NULL) {
                int fd = open(cmd->redirect_err, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror(cmd->redirect_err);
                    exit(1);
                }
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
            
            /* Close all pipe file descriptors */
            for (int j = 0; j < count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            /* Execute the command */
            execvp(cmd->com_pathname, cmd->argv);
            
            /* If execvp returns, there was an error */
            fprintf(stderr, "%s: command not found\n", cmd->com_pathname);
            exit(127);
        }
    }
    
    /* Parent process - close all pipes */
    for (i = 0; i < count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    /* Wait for all child processes if foreground */
    if (commands[start + count - 1].com_suffix != '&') {
        for (i = 0; i < count; i++) {
            waitpid(pids[i], &status, 0);
        }
    }
}

/* Expand wildcards in command arguments */
int expandWildcards(struct Command_struct *cmd) {
   
    int i, j;
    int new_argc = 0;
    char *new_argv[MAX_ARGS];
    int has_wildcards;
    
    for (i = 0; i < cmd->argc; i++) {
        has_wildcards = 0;
        
        /* Check if argument contains wildcards */
        for (j = 0; cmd->argv[i][j]; j++) {
            if (cmd->argv[i][j] == '*' || cmd->argv[i][j] == '?') {
                has_wildcards = 1;
                break;
            }
        }
        glob_t globbuf;
        memset(&globbuf, 0, sizeof(globbuf));
        
        if (has_wildcards) {
            /* Perform glob expansion */
            int flags = (i == 0) ? 0 : GLOB_APPEND;
            if (glob(cmd->argv[i], flags | GLOB_NOCHECK, NULL, &globbuf) == 0) {
                /* Add expanded filenames */
                for (size_t j = 0; j < globbuf.gl_pathc; j++) {
                    if (new_argc < MAX_ARGS - 1) {
                        new_argv[new_argc++] = strdup(globbuf.gl_pathv[j]);
                    }
                }
                
            } else {
                /* Expansion failed, keep original */
                if (new_argc < MAX_ARGS - 1) {
                    new_argv[new_argc++] = strdup(cmd->argv[i]);
                }
            }
        } else {
            /* No wildcards, keep original */
            if (new_argc < MAX_ARGS - 1) {
                new_argv[new_argc++] = strdup(cmd->argv[i]);
            }
        }
        globfree(&globbuf);
    }
    
    /* Replace old argv with expanded argv */
    if (new_argc > 0) {
        for (i = 0; i < cmd->argc; i++) {
            free(cmd->argv[i]);
        }
        for (i = 0; i < new_argc; i++) {
            cmd->argv[i] = new_argv[i];
        }
        cmd->argv[new_argc] = NULL;
        cmd->argc = new_argc;
        cmd->com_pathname = cmd->argv[0];
    }
    
    return 0;
}
