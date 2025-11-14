#include "shell.h"
#include <ctype.h>

/* Current position in history for arrow key navigation */
static int current_history_pos = -1;

/* Add command to history */
void addToHistory(char *line) {
    if (line == NULL || strlen(line) == 0) {
        return;
    }
    
    /* Avoid duplicate consecutive entries */
    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0) {
        return;
    }
    
    if (history_count < MAX_HISTORY) {
        history[history_count] = strdup(line);
        history_count++;
    } else {
        /* History is full, remove oldest entry */
        free(history[0]);
        
        /* Shift all entries */
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            history[i] = history[i + 1];
        }
        
        history[MAX_HISTORY - 1] = strdup(line);
    }
    
    history_index = history_count;
}

/* Get history command based on special syntax */
char *getHistoryCommand(char *input) {
    if (input[0] != '!') {
        return NULL;
    }
    
    /* !! - repeat last command */
    if (input[1] == '!' && input[2] == '\0') {
        if (history_count > 0) {
            return history[history_count - 1];
        }
        return NULL;
    }
    
    /* !n - repeat command number n */
    if (isdigit(input[1])) {
        int num = atoi(&input[1]);
        if (num > 0 && num <= history_count) {
            return history[num - 1];
        }
        return NULL;
    }
    
    /* !string - repeat last command starting with string */
    char *search_str = &input[1];
    int search_len = strlen(search_str);
    
    /* Search backwards through history */
    for (int i = history_count - 1; i >= 0; i--) {
        if (strncmp(history[i], search_str, search_len) == 0) {
            return history[i];
        }
    }
    
    return NULL;
}

/* Read line with arrow key support for history navigation */
char *readLineWithHistory(const char *prompt_str) {
    static char line_buffer[MAX_LINE_LENGTH];
    static int init_done = 0;
    static struct termios old_termios, new_termios;
    int pos = 0;
    int ch;
    int temp_history_pos;
    
    /* Initialize terminal settings first time */
    if (!init_done) {
        tcgetattr(STDIN_FILENO, &old_termios);
        new_termios = old_termios;
        new_termios.c_lflag &= ~(ICANON | ECHO);
        init_done = 1;
    }
    
    /* Set raw mode */
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    
    /* Print prompt */
    printf("%s ", prompt_str);
    fflush(stdout);
    
    /* Reset history position */
    temp_history_pos = history_count;
    line_buffer[0] = '\0';
    pos = 0;
    
    while (1) {
        ch = getchar();
        
        if (ch == '\n') {
            /* Enter pressed */
            printf("\n");
            line_buffer[pos] = '\0';
            tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
            current_history_pos = -1;
            return strdup(line_buffer);
        } else if (ch == 127 || ch == 8) {
            /* Backspace */
            if (pos > 0) {
                pos--;
                printf("\b \b");
                fflush(stdout);
            }
        } else if (ch == 4) {
            /* Ctrl-D (EOF) */
            if (pos == 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
                return NULL;
            }
        } else if (ch == 27) {
            /* Escape sequence */
            ch = getchar();
            if (ch == '[') {
                ch = getchar();
                if (ch == 'A') {
                    /* Up arrow */
                    if (temp_history_pos > 0) {
                        /* Clear current line */
                        while (pos > 0) {
                            printf("\b \b");
                            pos--;
                        }
                        
                        /* Get previous command from history */
                        temp_history_pos--;
                        strcpy(line_buffer, history[temp_history_pos]);
                        pos = strlen(line_buffer);
                        
                        /* Display it */
                        printf("%s", line_buffer);
                        fflush(stdout);
                    }
                } else if (ch == 'B') {
                    /* Down arrow */
                    if (temp_history_pos < history_count) {
                        /* Clear current line */
                        while (pos > 0) {
                            printf("\b \b");
                            pos--;
                        }
                        
                        temp_history_pos++;
                        if (temp_history_pos < history_count) {
                            /* Get next command from history */
                            strcpy(line_buffer, history[temp_history_pos]);
                            pos = strlen(line_buffer);
                            
                            /* Display it */
                            printf("%s", line_buffer);
                        } else {
                            /* At end of history, clear line */
                            line_buffer[0] = '\0';
                            pos = 0;
                        }
                        fflush(stdout);
                    }
                }
            }
        } else if (ch >= 32 && ch < 127) {
            /* Regular printable character */
            if (pos < MAX_LINE_LENGTH - 1) {
                line_buffer[pos++] = ch;
                putchar(ch);
                fflush(stdout);
            }
        }
    }
}
