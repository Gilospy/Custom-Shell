#include "token.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>

int tokenise(char *line, char *token[]) {
    int i = 0;
    char *ptr = line;
    char quote = 0;
    
    while (*ptr) {
        // Skip whitespace
        while (*ptr && isspace(*ptr)) {
            ptr++;
        }
        
        if (!*ptr) break;
        
        // Handle quoted strings
        if (*ptr == '\'' || *ptr == '"') {
            quote = *ptr;
            ptr++; // Skip opening quote
            char *start = ptr;
            
            // Find closing quote
            while (*ptr && *ptr != quote) {
                if (*ptr == '\\' && *(ptr+1) == quote) {
                    ptr++; // Skip escaped quote
                }
                ptr++;
            }
            
            if (*ptr != quote) {
                // Unclosed quote - error
                fprintf(stderr, "Error: Unmatched quote\n");
                return -1;
            }
            
            token[i] = strndup(start, ptr - start);
            if (!token[i]) {
                perror("strndup failed");
                return -1;
            }

            ptr++; // Skip closing quote
            i++;
            continue;
        }
        
        // Handle special characters (; | & < >)

        char *stderr_redirect = strstr(ptr, "2>");
if (stderr_redirect == ptr) {  // Ensure "2>" is at the current position
    token[i] = strndup(ptr, 2);
    if (!token[i]) {
        perror("strndup failed");
        return -1;
    }
    i++;
    ptr += 2; // Move past "2>"

    // Ensure there's a filename after "2>"
    while (*ptr && isspace(*ptr)) ptr++; // Skip spaces

    if (*ptr) {  // If there's a filename, tokenize it
        char *start = ptr;
        while (*ptr && !isspace(*ptr) && !strchr(";|&<>", *ptr)) {
            ptr++;
        }

        token[i] = strndup(start, ptr - start);
        if (!token[i]) {
            perror("strndup failed");
            return -1;
        }
        i++;
    } else {
        fprintf(stderr, "Error: Missing file after '2>'\n");
        return -1;
    }

    continue;
}
        if (strchr(";|&<>", *ptr)) {
            // Check for `2>` redirection
            /*if (*ptr == '2' && *(ptr+1) == '>') {
                token[i] = strndup(ptr, 2);
                ptr += 2;
            } 
            // Handle other single-character separators
            else {
                token[i] = strndup(ptr, 1);
                ptr++;
            }*/
            token[i] = strndup(ptr, 1);
                ptr++;

            if (!token[i]) {
                perror("strndup failed");
                return -1;
            }

            i++;
            continue;
        }
        
        // Handle regular tokens
        char *start = ptr;
        while (*ptr && !isspace(*ptr) && !strchr(";|&<>", *ptr)) {
            ptr++;
        }
        
        if (ptr > start) {
            token[i] = strndup(start, ptr - start);
            if (!token[i]) {
                perror("strndup failed");
                return -1;
            }
            i++;
        }
        
        if (i >= MAX_NUM_TOKENS) {
            fprintf(stderr, "Error: Too many tokens\n");
            return -1;
        }
    }
    
    token[i] = NULL;
    return i;
}