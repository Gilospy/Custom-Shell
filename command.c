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
#include "command.h"


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
            
          
            command[cmdCount].argv[command[cmdCount].argc] = tokens[i];
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
