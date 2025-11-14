#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <glob.h>
#include <pwd.h>
#include <termios.h>

/* Constants */
#define MAX_COMMANDS 100
#define MAX_ARGS 1000
#define MAX_LINE_LENGTH 10000
#define MAX_HISTORY 1000
#define DEFAULT_PROMPT "%"

/* Command structure */
struct Command_struct {
    char *com_pathname;      // path name of the command
    int argc;                // number of arguments including command
    char *argv[MAX_ARGS];    // argument array (NULL terminated)
    char *redirect_in;       // input redirection file (NULL if none)
    char *redirect_out;      // output redirection file (NULL if none)
    char *redirect_err;      // error redirection file (NULL if none)
    char com_suffix;         // ' ' (none), '&' (background), ';' (sequential), '|' (pipe)
};

/* Global variables */
extern char current_prompt[256];
extern char *history[MAX_HISTORY];
extern int history_count;
extern int history_index;

/* Function prototypes */

/* Parser functions */
int parseCommandLine(char *line, struct Command_struct commands[]);
void freeCommands(struct Command_struct commands[], int num_commands);
void printComStruct(struct Command_struct *com);

/* Built-in command functions */
int isBuiltIn(char *command);
int executeBuiltIn(struct Command_struct *cmd);
void builtInPrompt(char *new_prompt);
void builtInPWD(void);
void builtInCD(char *path);
void builtInHistory(void);
void builtInExit(void);

/* Execution functions */
void executeCommands(struct Command_struct commands[], int num_commands);
void executeSingleCommand(struct Command_struct *cmd);
void executePipeline(struct Command_struct commands[], int start, int count);

/* Wildcard expansion */
int expandWildcards(struct Command_struct *cmd);

/* History management */
void addToHistory(char *line);
char *getHistoryCommand(char *input);

/* Line editing with arrow key support */
char *readLineWithHistory(const char *prompt_str);

/* Signal handlers */
void setupSignalHandler(void);
void sigchildHandler();

/* Utility functions */
char *trimWhitespace(char *str);
int isSpecialChar(char c);

#endif /* SHELL_H */
