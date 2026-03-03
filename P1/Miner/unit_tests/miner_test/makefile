CC=gcc
EXE=miner_test
CFLAGS=-c -Wall -Wextra -std=gnu99 -pedantic -g

###########################################################################

$(EXE): miner_test.o miner.o logger.o pow.o
	$(CC) -o $(EXE) $^

%.o: %.c
	$(CC) $(CFLAGS) $^

run:
	$(MAKE); valgrind --leak-check=full ./$(EXE)

clean:
	rm -rf *.o $(EXE)
