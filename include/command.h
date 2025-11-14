
#ifndef COMMAND_H
#define COMMAND_H


#define MAX_NUM_TOKENS 1000


// command separators
#define pipeSep  "|"  // pipe separator "|"
#define conSep   "&"  // concurrent execution separator "&"
#define seqSep   ";"  // sequential execution separator ";"
#define inputSep "<"  // input redirection "<"
#define outputSep ">" // output redirection ">"
#define errorSep "2>" // error redirection "2>"
#define wildcard  "*"  // wildcard "*"
#define wildcardSingle "?" // single-character wildcard "?"

#define CD_CMD "cd" // change directory command
#define EXIT_CMD "exit"    // exit command
#define PROMPT_CMD "prompt" // change prompt command
#define PWD_CMD "pwd" // print working directory command
#define HISTORY_CMD "history" // history command

struct CommandStruct {
   char* command; // The command name (first token of the command)
   char *separator;	   // the command separator that follows the command,  must be one of "|", "&", and ";"
   int argc;       // number of arguments (tokens) in the command 
   char *argv[MAX_NUM_TOKENS];     // array of pointers to the command's argument tokens
   char* redirectTo;   // Redirect output or error to this file
   int redirectType;  // 0: none, 1: input, 2: output, 3: error
 };

typedef struct CommandStruct Command;


/**
 * Separate tokens into commands based on separators.
 * @param tokens Array of token strings.
 * @param command Array to store separated commands.
 * @return Count of commands separate or error code.
 * Returns the number of commands found, or negative error codes:
 * -1: first token is a separator
 * -2: consecutive separators
 * -3: last command ends with pipe separator
 * -4: too many arguments in a command
 *
 */
int separateCommands(char *tokens[], Command command[]);

#endif // COMMAND_H