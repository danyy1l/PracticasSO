EXE=miner
LOGGER=logger
CC=cc
CFLAGS=-c -Wall -Wextra -std=c99 -pedantic -g

all: $(EXE) $(LOGGER)

OBJ = $(EXE).o $(LOGGER).o pow.o

$(EXE): $(EXE).o pow.o
	$(CC) -o $@ $^

$(LOGGER): $(LOGGER).o
	$(CC) -o $@ $^

%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(EXE) $(LOGGER)
