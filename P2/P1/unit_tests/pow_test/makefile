CC=cc
EXE=pow_test
CFLAGS=-c -Wall -Wextra -std=gnu99 -pedantic -g

###########################################################################

$(EXE): pow_test.o pow.o
	$(CC) -o $(EXE) *.o

pow_test.o: pow_test.c
	$(CC) $(CFLAGS) $^

pow.o: pow.c
	$(CC) $(CFLAGS) $^

run:
	$(MAKE); valgrind --leak-check=full ./$(EXE)

clean:
	rm -rf *.o $(EXE)
