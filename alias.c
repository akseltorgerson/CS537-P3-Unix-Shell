#include "alias.h"
#include "mysh.h"

struct listNode_t* head = NULL;
struct listNode_t* currNode = NULL;

bool addAlias(char* aliasName, char* command) {
  struct listNode_t* newNode;
  newNode = (struct listNode_t*)malloc(sizeof(struct listNode_t));
  freeList[numFreePtr++].listNodePtr = newNode;
  newNode->alias = aliasName;
  newNode->command = command;
  newNode->next = NULL;
  if (head == NULL) {
    head = newNode;
  } else {
    currNode = head;
    while (currNode->next != NULL) {currNode = currNode->next;}
    currNode->next = newNode;
  }
  return true;
}

bool unalias(char* aliasName) {
  currNode = head;
  struct listNode_t* prevNode = NULL;
  if (head == NULL) {
    return false;
  } else {
    while (strcmp(currNode->alias, aliasName) != 0) {
      if (currNode->next != NULL) {
        prevNode = currNode;
        currNode = currNode->next;
      } else {
        return false;
      }
    }

    if (currNode == head) {
      head = head->next;
    } else {
      prevNode->next = currNode->next;
    }
    return true;
  }
}

char* decodeAlias(char* alias) {
  currNode = NULL;
  if (head == NULL) {
    return NULL;
  } else {
    currNode = head;
    while (strcmp(currNode->alias, alias) != 0) {
      if (currNode->next != NULL) {
        currNode = currNode->next;
      } else {
        return NULL;
      }
    }
  }
  return currNode->command;
}

void listAlias() {
  currNode = head;
  while (currNode != NULL) {
    printf("%s %s\n", currNode->alias, currNode->command);
    fflush(stdout);
    currNode = currNode->next;
  }
}
