#include "token.h"
#include <string.h>
#include <stdio.h>

int tokenise(char *inputLine, char *token[]) {
    int count = 0;
    const char *delim = " \t\n"; // whitespace characters

    char *tok = strtok(inputLine, delim);
    while (tok != NULL) {
        // Reserve one slot for the NULL terminator so callers can detect the end
        if (count >= MAX_NUM_TOKENS - 1) {
            // Too many tokens to fit along with a NULL terminator
            token[count] = NULL;
            count = -1;
        }
        token[count++] = tok;
        tok = strtok(NULL, delim);
    }
    if(count == -1) {
        return -1; // Indicate error due to too many tokens
    }
    else{
        token[count] = NULL; // NULL-terminate the token list
        return count; // Return the number of tokens found
    }
}
