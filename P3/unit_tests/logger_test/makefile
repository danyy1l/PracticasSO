CC = gcc
SRC_DIR = ../..
VPATH = $(SRC_DIR)

# Añadimos -I..
CFLAGS = -Wall -Wextra -std=gnu99 -pedantic -g -I$(SRC_DIR) -I..
CLIBS = -lpthread

EXE = logger_test
OBJS = logger_test.o logger.o files.o

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(CLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f *.o $(EXE)
