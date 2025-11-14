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
#include <stdio.h>
#include <string.h>
#include "token.h"
#include "command.h"
#include "history.h"
#include "signals.h"
#include "executor.h"

#define MAX_LINE 1024 /* maximum length of input line */

int main(void) {
    char line[MAX_LINE];
    char prompt[64] = "%";
    char *history[MAX_HISTORY] = {0};
    int historyCount = 0;

    /* Install signal handlers used by the shell */
    if (setupSignalHandlers() != 0) {
        fprintf(stderr, "Failed to setup signal handlers\n");
        return 1;
    }

    while (1) {
        printf("%s ", prompt);
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';

        /* Skip empty input */
        if (strlen(line) == 0) continue;

        /* History expansion (e.g. !!, !n, !prefix) */
        if (line[0] == '!') {
            char expandedLine[MAX_LINE];
            if (handleHistoryExpansion(line, expandedLine, history, historyCount) == -1) {
                continue; /* expansion failed or not found */
            }
            strcpy(line, expandedLine);
            printf("%s\n", line); /* echo expanded command */
        }

        /* Execute the (possibly expanded) command line */
        executeCommandLine(line, prompt, history, &historyCount);
    }

    return 0;
}