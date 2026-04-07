CC = gcc
SRC_DIR = ../..
VPATH = $(SRC_DIR)

CFLAGS = -Wall -Wextra -std=gnu99 -pedantic -g -I$(SRC_DIR) -I..
CLIBS = -lpthread

EXE = files_test
OBJS = files_test.o files.o

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(CLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f *.o $(EXE)
