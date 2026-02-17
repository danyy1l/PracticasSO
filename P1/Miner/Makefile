EXE=miner
LOGGER=logger
CC=cc
CFLAGS=-Wall -Wextra -std=c99 -pedantic -g
CLIBS=-lpthread

.PHONY: clean run

all: $(EXE) $(LOGGER)

$(EXE): $(EXE).o pow.o
	$(CC) -o $@ $^ $(CLIBS)

$(LOGGER): $(LOGGER).o
	$(CC) -o $@ $^

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXE) $(LOGGER)

run: all
	./$(EXE) 1 2 3
