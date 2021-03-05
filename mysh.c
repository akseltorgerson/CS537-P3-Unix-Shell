#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "alias.h"
#include "mysh.h"

heapTracker_t freeList[99999];
int numFreePtr = 0;

void* freeMem() {
  for (int i = 0; i < numFreePtr; i++) {
    if (freeList[i].ptr != NULL) {
      free(freeList[i].ptr);
    } else if (freeList[i].intPtr != NULL) {
      free(freeList[i].intPtr);
    } else if (freeList[i].listNodePtr != NULL) {
      free(freeList[i].listNodePtr);
    } else {
      free(freeList[i].argvPtr);
    }
  }
}

/***************************************************************************************
* This function prints different errors based on an arbitrary integer passed to it.
***************************************************************************************/
void printError(char* data) {
  write(STDERR_FILENO, data, strlen(data));
}

/***************************************************************************************
* This function parses a command line. Returns a struct that holds argv and argc for
* the desired command.
***************************************************************************************/
argument_t processCommand(char* cmdLine) {
  char *token;
  char *cpyToken;
  char **argv = (char**)malloc(sizeof(char*) * MAX_CMD_TOKN);
  freeList[numFreePtr++].argvPtr = argv;
  int argc = 0;
  argument_t retVal;
  retVal.redir = false;
  retVal.valid = true;
  // get rid of tabs, check if multiple redirect characters
  // replace newline with end of string
  for (int i = 0; i < strlen(cmdLine); i++) {
    if (cmdLine[i] == '\t') {
      cmdLine[i] = ' ';
    } else if (cmdLine[i] == '>') {
      if (retVal.redir == false) {
        retVal.redir = true;
      } else {
        printError("Redirection misformatted.\n");
        retVal.valid = false;
        return retVal;
      }
    } else if (cmdLine[i] == '\n') {
      cmdLine[i] = '\0';
    }
  }
  // make a duplication of cmdLine
  char* cpyLine = strdup(cmdLine);
  freeList[numFreePtr++].ptr = cpyLine;
  // get the first token of the string
  token = strtok(cpyLine, " ");
  // loop through all the rest of the tokens, adding them to argv as we go
  while (token != NULL) {
    argv[argc] = strdup(token);
    freeList[numFreePtr++].ptr = argv[argc];
    token = strtok(NULL, " ");
    argc++;
  }
  // at this point we know if redir is true, there is ONE redirection character
  // we need to make sure that if that character is the only char in the token,
  // that it doesnt come in the first or last token
  // Really should be using strchr here and strrchr
  if (retVal.redir) {
    if (strcmp(argv[0], ">") == 0 || strcmp(argv[argc - 1], ">") == 0) {
      printError("Redirection misformatted.\n");
      retVal.valid = false;
      return retVal;
    } else {
      // it could be in the first or last token ex /usr/bin/ls>out.txt
      // or /usr/bin/echo hello world>out.txt
      for (int i = 0; i < argc; i++) {
        // loop through every token
        if (strstr(argv[i], ">") != NULL) {
          if (strlen(argv[i]) == 1) {
            // the token is only >
            // therefore the next token should be the filename
            if (i != argc - 2) {
              printError("Redirection misformatted.\n");
              retVal.valid = false;
              return retVal;
            }
            argv[i] = argv[i + 1];
            argc--;
          } else {
            // this token contains the ONE and ONLY > in the command
            // the contents AFTER > should be the filename we want to redirect
            // to the contents BEFORE > should be the last command we want to
            // execute check if the > is at the beggining of the token
            if (argv[i][0] == '>') {
              // point to the character right after it
              argv[i] = argv[i] + sizeof(char);

            } else if (argv[i][strlen(argv[i]) - 1] == '>') {
              // replace that character
              argv[i][strlen(argv[i]) - 1] = '\0';
            } else {
              // the redirect character is in the middle of a token
              // create a copy of the token
              cpyToken = strdup(argv[i]);
              freeList[numFreePtr++].ptr = cpyToken;
              char* lastToken;
              char* filename;

              // tokenize the token around ">"
              lastToken = strtok(cpyToken, ">");
              filename = strtok(NULL, ">");

              argv[i] = lastToken;
              argv[i + 1] = filename;
              argc++;
            }
          }
        }
      }
    }
  }
  // need tokens to be null terminated
  argv[argc] = NULL;
  if (argv[0] == NULL) {
    // empty command
    retVal.valid = false;
  }
  // put argv and argc into a struct so both can be returned
  retVal.argv = argv;
  retVal.argc = argc;
  // command is all tokenized in argv, with argc holding the # of tokens
  return retVal;
}

char* concat(int size, char** strings) {
  int* lengths = (int*)malloc(sizeof(int) * size);
  freeList[numFreePtr++].intPtr = lengths;
  int totalSize = 0;
  char* final;
  char* ptr;
  for (int i = 2; i < size; i++) {
    lengths[i] = strlen(strings[i]);
    totalSize += strlen(strings[i]);
  }
  totalSize += (size);
  final = ptr = malloc(totalSize);
  freeList[numFreePtr++].ptr = ptr;
  for (int i = 2; i < size; i++) {
    strcpy(ptr, strings[i]);
    ptr += lengths[i];
    if (i < size - 1) {
      strcpy(ptr, " ");
      ptr++;
    }
  }
  return final;
}

void executeAlias(argument_t arg) {
  if (arg.argc == 1) {
    listAlias();
  } else if (arg.argc == 2) {
    char* command = decodeAlias(arg.argv[1]);
    if (command != NULL) {
      fprintf(stdout, "%s %s\n", arg.argv[1], command);
      fflush(stdout);
    }
  } else if (strcmp(arg.argv[1], "alias") == 0 ||
            strcmp(arg.argv[1], "unalias") == 0 ||
            strcmp(arg.argv[1], "exit") == 0 ) {
    printError("alias: Too dangerous to alias that.\n");
  } else {
    // need to concat all the tokens
    char* command = concat(arg.argc, arg.argv);
    if (decodeAlias(arg.argv[1]) != NULL) {
      unalias(arg.argv[1]);
      addAlias(arg.argv[1], command);
    } else {
      addAlias(arg.argv[1], command);
    }
  }
}

void executeUnalias(argument_t arg) {
  if (arg.argc != 2) {
    printError("unalias: Incorrect number of arguments.\n");
  } else {
    unalias(arg.argv[1]);
  }
}

/***************************************************************************************
* This is the function that actually executes the command.
***************************************************************************************/
void executeCommand(argument_t arg, FILE* batchFile) {
  int pid;
  if (strcmp(arg.argv[0], "exit") == 0) {
    if (batchFile != NULL) {
      fclose(batchFile);
      batchFile = NULL;
    }
    freeMem();
    _exit(0);
  } else if (strcmp(arg.argv[0], "alias") == 0) {
      executeAlias(arg);
  } else if (strcmp(arg.argv[0], "unalias") == 0) {
      executeUnalias(arg);
  } else if (strcmp(arg.argv[0], "") != 0) {
    pid = fork();
    if (pid < 0) {
      // only time pid can be negative is if fork has failed
      printError("Fork has failed!\n");
    } else if (pid == 0) {
      // * CHILD PROCESS
      // check for redirection
      if (arg.redir) {
        char* filename = arg.argv[arg.argc - 1];
        FILE* fp = fopen(filename, "w");
        if (fp == NULL) {
          fprintf(stderr, "Error: Cannot write to file %s.\n", filename);
          fflush(stderr);
          _exit(0);
        } else {
          dup2(fileno(fp), STDOUT_FILENO);
          // now that we have the file setup go ahead and NULL out that file
          // in argv so that execv will run properly
          arg.argv[arg.argc - 1] = NULL;
        }
      }
      // check for aliasing
      char* alias = decodeAlias(arg.argv[0]);
      if (alias != NULL) {
        argument_t aliasArg = processCommand(alias);
        for (int i = 1; i <= arg.argc; i++) {
          aliasArg.argv[aliasArg.argc] = arg.argv[i];
          aliasArg.argc++;
        }
        execv(aliasArg.argv[0], aliasArg.argv);
        printError(strcat(aliasArg.argv[0], ": Command not found.\n"));
        _exit(0);
      }
      execv(arg.argv[0], arg.argv);
      // we shouldnt get to this point of the code, if we do execv failed
      printError(strcat(arg.argv[0], ": Command not found.\n"));
      _exit(0);
    } else {
      // * PARENT PROCESS
      waitpid(pid, NULL, 0);
    }
  }
}

/***************************************************************************************
* Main method. Calls all helper functions from here.
***************************************************************************************/
int main(int argc, char* argv[]) {
  char cmdLine[MAX_CMD_CHAR];
  argument_t commandArg;
  if (argc == 2) {
    // * Batch Mode
    FILE* batchFile = fopen(argv[1], "r");
    if (batchFile == NULL) {
      fprintf(stderr, "Error: Cannot open file %s.\n", argv[1]);
      fflush(stderr);
      exit(1);
    } else {
      // file successfully open
      while (fgets(cmdLine, MAX_CMD_CHAR, batchFile) != NULL) {
        // echo line to stdout
        write(STDOUT_FILENO, cmdLine, strlen(cmdLine));
        // parse the command
        if (strlen(cmdLine) < MAX_CMD_CHAR - 1) {
          commandArg = processCommand(cmdLine);
          // execute the command
          if (commandArg.valid == true) {
            executeCommand(commandArg, batchFile);
          }
        }
      }
    }
  } else if (argc == 1) {
    // * Interactive Mode
    char* shellPrompt = "mysh> ";
    while (write(STDOUT_FILENO, shellPrompt, strlen(shellPrompt)) &&
    fgets(cmdLine, MAX_CMD_CHAR, stdin)) {
      if (strlen(cmdLine) == MAX_CMD_CHAR - 1) {
        printError("Command too long!\n");
      } else if (strcmp(cmdLine, "\n") != 0) {
        commandArg = processCommand(cmdLine);
        // execute the command
        if (commandArg.valid == true) {
          executeCommand(commandArg, NULL);
        }
      }
    }
  } else {
    // * Incorrect # of argumenmts
    printError("Usage: mysh [batch-file]\n");
    exit(1);
  }
}
