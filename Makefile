# Build by running 'make'

# Standard Variables
CC = gcc
FILES = mysh.c alias.c

# Compiler Flags
CFLAGS = -Wall -Werror -g
DFLAGS = -g -ggdb

mysh : mysh.c alias.c
	$(CC) $(DFLAGS) -o mysh $(FILES)

.PHONY: clean
clean :
	rm -rf mysh *.o
	rm -rf test* 
