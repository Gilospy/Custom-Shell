#include <stdio.h>
#include <string.h>
#include "token.h"
#include "command.h"

#define MAX_LINE 1024 // maximum length of input line
#define MAX_NUM_COMMANDS 100

int main() {
    char line[MAX_LINE];
    char *tokens[MAX_NUM_TOKENS];
    Command commands[MAX_NUM_COMMANDS];
    int tokenCount, cmdCount;
    char prompt[64] = "$ ";

    while (1) {
        printf("%s", prompt);
        if (!fgets(line, sizeof(line), stdin)) break;

        if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';
        if (strcmp(line, "exit") == 0) break;

        // Tokenize input line
        tokenCount = tokenise(line, tokens);
        if (tokenCount == -1) {
            printf("Too many tokens!\n");
            continue;
        }

        // Separate commands
        cmdCount = separateCommands(tokens, commands);

        if (cmdCount == -1) {
            printf("Syntax error: command cannot start with a separator.\n");
            continue;
        }
        else if (cmdCount == -2) {
            printf("Syntax error: Consecutive Separators.\n");
            continue;
        }
        else if (cmdCount == -3) {
            printf("Syntax error: last command cannot end with '|'.\n");
            continue;
        }
        else if (cmdCount == -4) {
            printf("Error: Too many arguments in a command.\n");
            continue;
        }

        // Print commands and their tokens
        for (int i = 0; i < cmdCount; i++) {
            printf("Command %d:\n", i);
            printf("  Command name: %s\n", commands[i].command);
            printf("  Argc: %d\n", commands[i].argc);
            printf("  Arguments:\n");
            for (int j = 0; j < commands[i].argc; j++) {
                printf("    argv[%d]: %s\n", j, commands[i].argv[j]);
            }
            if (commands[i].separator)
                printf("  Separator: %s\n", commands[i].separator);
            else
                printf("  Separator: (none)\n");
        }
    }
    
    return 0;
}
