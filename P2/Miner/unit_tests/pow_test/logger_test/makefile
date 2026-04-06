CC=gcc
EXE=logger_test
CFLAGS=-c -Wall -Wextra -std=gnu99 -pedantic -g

all: $(EXE)

$(EXE): logger_test.o logger.o files.o
	$(CC) -o $(EXE) logger_test.o logger.o files.o -pthread

%.o: %.c
	$(CC) $(CFLAGS) $<

clean:
	rm -rf *.o $(EXE) *.log
