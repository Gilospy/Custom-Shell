#include "shell.h"

/* Check if command is a built-in */
int isBuiltIn(char *command) {
    if (strcmp(command, "prompt") == 0) return 1;
    if (strcmp(command, "pwd") == 0) return 1;
    if (strcmp(command, "cd") == 0) return 1;
    if (strcmp(command, "history") == 0) return 1;
    if (strcmp(command, "exit") == 0) return 1;
    return 0;
}

/* Execute built-in command */
int executeBuiltIn(struct Command_struct *cmd) {
    char *command = cmd->com_pathname;
    
    if (strcmp(command, "prompt") == 0) {
        if (cmd->argc >= 2) {
            builtInPrompt(cmd->argv[1]);
        } else {
            fprintf(stderr, "prompt: missing argument\n");
        }
        return 1;
    }
    
    if (strcmp(command, "pwd") == 0) {
        builtInPWD();
        return 1;
    }
    
    if (strcmp(command, "cd") == 0) {
        if (cmd->argc >= 2) {
            builtInCD(cmd->argv[1]);
        } else {
            builtInCD(NULL);  /* Go to home directory */
        }
        return 1;
    }
    
    if (strcmp(command, "history") == 0) {
        builtInHistory();
        return 1;
    }
    
    if (strcmp(command, "exit") == 0) {
        builtInExit();
        return 1;
    }
    
    return 0;
}

/* Change shell prompt */
void builtInPrompt(char *new_prompt) {
    if (new_prompt != NULL && strlen(new_prompt) < sizeof(current_prompt)) {
        strcpy(current_prompt, new_prompt);
    }
}

/* Print working directory */
void builtInPWD(void) {
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

/* Change directory */
void builtInCD(char *path) {
    char *target_path;
    
    if (path == NULL) {
        /* No argument - go to home directory */
        target_path = getenv("HOME");
        if (target_path == NULL) {
            struct passwd *pw = getpwuid(getuid());
            target_path = pw->pw_dir;
        }
    } else {
        target_path = path;
    }
    
    if (chdir(target_path) != 0) {
        perror("cd");
    }
}

/* Display command history */
void builtInHistory(void) {
    int i;
    for (i = 0; i < history_count; i++) {
        printf("%d  %s\n", i + 1, history[i]);
    }
}

/* Exit shell */
void builtInExit(void) {
    int i;
    
    /* Free history */
    for (i = 0; i < history_count; i++) {
        free(history[i]);
    }
    
    printf("exit\n");
    exit(0);
}
