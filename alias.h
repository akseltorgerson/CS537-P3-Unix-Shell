#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct listNode_t {
  char* alias;
  char* command;
  struct listNode_t* next;
};

bool addAlias(char* aliasName, char* command);

bool unalias(char* aliasName);

char* decodeAlias(char* aliasName);

void listAlias(void);
