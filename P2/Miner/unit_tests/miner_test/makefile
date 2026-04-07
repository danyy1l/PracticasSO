CC = gcc
SRC_DIR = ../..
VPATH = $(SRC_DIR)

# Añadimos -I..
CFLAGS = -Wall -Wextra -std=gnu99 -pedantic -g -I$(SRC_DIR) -I..
CLIBS = -lpthread -lrt 

EXE = miner_test
OBJS = miner_test.o miner.o files.o logger.o pow.o

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(CLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f *.o $(EXE)
