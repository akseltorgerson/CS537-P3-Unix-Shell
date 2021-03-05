// Macros
#define MAX_CMD_CHAR 512
#define MAX_CMD_TOKN 100

// Structs
typedef struct arg {
  char** argv;
  int argc;
  bool redir;
  bool valid;
} argument_t;

typedef struct heapTracker {
  int* intPtr;
  char* ptr;
  char** argvPtr;
  struct listNode_t* listNodePtr;
} heapTracker_t;

heapTracker_t freeList[99999];
int numFreePtr;
