CC=gcc
EXE=files_test
CFLAGS=-c -Wall -Wextra -std=gnu99 -pedantic -g

all: $(EXE)

$(EXE): files_test.o files.o
	$(CC) -o $(EXE) files_test.o files.o -pthread

%.o: %.c
	$(CC) $(CFLAGS) $<

clean:
	rm -rf *.o $(EXE) test_votes.dat test_target.dat
