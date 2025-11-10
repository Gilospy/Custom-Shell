#include <stdio.h>
#include <string.h>
#include "token.h"
#include "command.h"

int main() {
    char input[100];
    char *token[MAX_NUM_TOKENS];
    Command command[MAX_NUM_COMMANDS];
    int ntokens, ncommands;

    while (1) {
        printf("%% ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        if (strcmp(input, "exit") == 0) {
            break;
        }

        // Tokenize the input line
        ntokens = tokenise(input, token);
        if (ntokens == -1) {
            printf("Error: too many tokens (limit %d)\n", MAX_NUM_TOKENS);
            continue;
        }

        // Separate tokens into commands
        ncommands = separateCommands(token, command);

        // Handle possible syntax or structural errors
        if (ncommands < 0) {
            switch (ncommands) {
                case -2:
                    printf("Syntax error: consecutive separators.\n");
                    break;
                case -3:
                    printf("Syntax error: command cannot start with a separator.\n");
                    break;
                case -4:
                    printf("Syntax error: last command cannot end with '|'.\n");
                    break;
                default:
                    printf("Unknown error while separating commands.\n");
            }
            continue;
        }

        // Display parsed commands and their tokens
        printf("Found %d command(s):\n", ncommands);
        for (int i = 0; i < ncommands; i++) {
            printf("  Command %d:\n", i + 1);
            for (int j = command[i].first; j <= command[i].last; j++) {
                if (token[j] != NULL) {
                    printf("    token[%d] = \"%s\"\n", j, token[j]);
                }
            }
            printf("    Separator: \"%s\"\n", command[i].sep);
        }
    }

    return 0;
}
