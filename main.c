#include "shell.h"

/* Global variables */
char current_prompt[256] = DEFAULT_PROMPT;
char *history[MAX_HISTORY];
int history_count = 0;
int history_index = -1;

int main() {
    char *line;
    struct Command_struct commands[MAX_COMMANDS];
    int num_commands;
    
    /* Setup signal handlers */
    setupSignalHandler();
    
    /* Main shell loop */
    while (1) {
        /* Read command line with arrow key support */
        line = readLineWithHistory(current_prompt);
        
        if (line == NULL) {
            /* EOF or error */
            printf("\n");
            builtInExit();
        }
        
        /* Skip empty lines */
        if (strlen(trimWhitespace(line)) == 0) {
            free(line);
            continue;
        }
        
        /* Handle history commands */
        if (line[0] == '!') {
            char *history_cmd = getHistoryCommand(line);
            if (history_cmd == NULL) {
                fprintf(stderr, "History command not found\n");
                free(line);
                continue;
            }
            free(line);
            line = strdup(history_cmd);
            printf("%s\n", line);
        }
        
        /* Add to history */
        addToHistory(line);
        
        /* Parse command line */
        num_commands = parseCommandLine(line, commands);
        if (num_commands < 0) {
            fprintf(stderr, "Error parsing command line\n");
            free(line);
            continue;
        }
        
        if (num_commands == 0) {
            free(line);
            continue;
        }
        
        /* Execute commands */
        executeCommands(commands, num_commands);
        
        /* Free allocated memory */
        freeCommands(commands, num_commands);
        free(line);
    }
    
    return 0;
}

/* Print command structure for debugging */
void printComStruct(struct Command_struct *com) {
    int i;
    
    fprintf(stderr, "com_pathname=%s\n", com->com_pathname);
    fprintf(stderr, "argc=%d\n", com->argc);
    for (i = 0; com->argv[i] != NULL; i++)
        fprintf(stderr, "argv[%d]=%s\n", i, com->argv[i]);
    fprintf(stderr, "#######\n");
    if (com->redirect_in == NULL)
        fprintf(stderr, "redirect_in=NULL\n");
    else
        fprintf(stderr, "redirect_in=%s\n", com->redirect_in);
    if (com->redirect_out == NULL)
        fprintf(stderr, "redirect_out=NULL\n");
    else
        fprintf(stderr, "redirect_out=%s\n", com->redirect_out);
    if (com->redirect_err == NULL)
        fprintf(stderr, "redirect_err=NULL\n");
    else
        fprintf(stderr, "redirect_err=%s\n", com->redirect_err);
    fprintf(stderr, "com_suffix=%c\n\n", com->com_suffix);
}
