#include "shell.h"
#include <ctype.h>

/* Helper function to check if character is a special character */
int isSpecialChar(char c) {
    return (c == '&' || c == ';' || c == '|' || 
            c == '<' || c == '>' || c == '\'' || 
            c == '"' || c == '!');
}

/* Trim leading and trailing whitespace */
char *trimWhitespace(char *str) {
    char *end;
    
    /* Trim leading space */
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    *(end + 1) = 0;
    return str;
}

/* Parse a single token from the command line */
static char *parseToken(char **line_ptr) {
    char *start = *line_ptr;
    char *token;
    int len = 0;
    int in_single_quote = 0;
    int in_double_quote = 0;
    char buffer[MAX_LINE_LENGTH];
    
    /* Skip leading whitespace */
    while (*start && isspace(*start)) start++;
    
    if (*start == '\0') {
        *line_ptr = start;
        return NULL;
    }
    
    /* Check for special tokens */
    if (!in_single_quote && !in_double_quote) {
        /* Check for 2> (stderr redirection) */
        if (*start == '2' && *(start + 1) == '>') {
            buffer[0] = '2';
            buffer[1] = '>';
            buffer[2] = '\0';
            *line_ptr = start + 2;
            token = strdup(buffer);
            return token;
        }
        
        /* Check for other special single-character tokens */
        if (*start == '&' || *start == ';' || *start == '|') {
            buffer[0] = *start;
            buffer[1] = '\0';
            *line_ptr = start + 1;
            token = strdup(buffer);
            return token;
        }
        if (*start == '<' || *start == '>') {
            buffer[0] = *start;
            buffer[1] = '\0';
            *line_ptr = start + 1;
            token = strdup(buffer);
            return token;
        }
    }
    
    /* Parse regular token */
    char *p = start;
    while (*p) {
        if (*p == '\\' && *(p + 1)) {
            /* Escaped character */
            buffer[len++] = *(p + 1);
            p += 2;
        } else if (*p == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            p++;
        } else if (*p == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            p++;
        } else if (!in_single_quote && !in_double_quote && 
                   (isspace(*p) || isSpecialChar(*p))) {
            break;
        } else {
            buffer[len++] = *p;
            p++;
        }
    }
    
    buffer[len] = '\0';
    *line_ptr = p;
    
    if (len == 0) return NULL;
    
    token = strdup(buffer);
    return token;
}

/* Parse command line into array of command structures */
int parseCommandLine(char *line, struct Command_struct commands[]) {
    char *p = line;
    char *token;
    int cmd_index = 0;
    int arg_index;
    char last_suffix = ' ';
    
    /* Initialize commands array */
    memset(commands, 0, sizeof(struct Command_struct) * MAX_COMMANDS);
    
    while (*p && cmd_index < MAX_COMMANDS) {
        /* Initialize current command */
        commands[cmd_index].com_pathname = NULL;
        commands[cmd_index].argc = 0;
        commands[cmd_index].redirect_in = NULL;
        commands[cmd_index].redirect_out = NULL;
        commands[cmd_index].redirect_err = NULL;
        commands[cmd_index].com_suffix = ' ';
        arg_index = 0;
        
        /* Parse tokens for this command */
        while ((token = parseToken(&p)) != NULL) {
            /* Check for special tokens */
            if (strcmp(token, "&") == 0 || strcmp(token, ";") == 0 || 
                strcmp(token, "|") == 0) {
                commands[cmd_index].com_suffix = token[0];
                free(token);
                break;
            } else if (strcmp(token, "<") == 0) {
                free(token);
                token = parseToken(&p);
                if (token) {
                    commands[cmd_index].redirect_in = token;
                }
            } else if (strcmp(token, ">") == 0) {
                free(token);
                token = parseToken(&p);
                if (token) {
                    commands[cmd_index].redirect_out = token;
                }
            } else if (strcmp(token, "2>") == 0) {
                /* Handle stderr redirection */
                free(token);
                token = parseToken(&p);
                if (token) {
                    commands[cmd_index].redirect_err = token;
                }
            } else {
                /* Regular argument */
                if (arg_index < MAX_ARGS - 1) {
                    commands[cmd_index].argv[arg_index++] = token;
                    commands[cmd_index].argc++;
                    if (commands[cmd_index].com_pathname == NULL) {
                        commands[cmd_index].com_pathname = token;
                    }
                }
            }
        }
        
        /* Null-terminate argv */
        commands[cmd_index].argv[arg_index] = NULL;
        
        /* Check if we have a valid command */
        if (commands[cmd_index].argc > 0) {
            cmd_index++;
        }
        
        /* Check if we're done */
        if (last_suffix != '|' && commands[cmd_index - 1].com_suffix != '|') {
            if (commands[cmd_index - 1].com_suffix == '&' || 
                commands[cmd_index - 1].com_suffix == ';') {
                last_suffix = commands[cmd_index - 1].com_suffix;
            } else {
                break;
            }
        }
    }
    
    return cmd_index;
}

/* Free allocated memory in commands array */
void freeCommands(struct Command_struct commands[], int num_commands) {
    int i, j;
    
    for (i = 0; i < num_commands; i++) {
        for (j = 0; j < commands[i].argc; j++) {
            if (commands[i].argv[j] != NULL && 
                commands[i].argv[j] != commands[i].com_pathname) {
                free(commands[i].argv[j]);
            }
        }
        if (commands[i].com_pathname != NULL) {
            free(commands[i].com_pathname);
        }
        if (commands[i].redirect_in != NULL) {
            free(commands[i].redirect_in);
        }
        if (commands[i].redirect_out != NULL) {
            free(commands[i].redirect_out);
        }
        if (commands[i].redirect_err != NULL) {
            free(commands[i].redirect_err);
        }
    }
}
