EXE=mrush
CC=cc
CFLAGS=-Wall -Wextra -std=gnu99 -pedantic -g
CLIBS=-lpthread

OBJ_DIR = obj

.PHONY: clean run doc test_threads test_rounds test_all

HEADERS=miner.h pow.h logger.h types.h
OBJ= $(addprefix $(OBJ_DIR)/, pow.o logger.o miner.o $(EXE).o)

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(CLIBS)

$(OBJ_DIR)/%.o : %.c $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	-rm -rf *.log
	rm -rf DocumentacionMinero.pdf doc $(OBJ_DIR) $(EXE) test_logs
run: all
	./$(EXE) 1 100 5

doc:
	doxygen Doxyfile
	$(MAKE) -C doc/latex
	cp doc/latex/refman.pdf ./DocumentacionMinero.pdf

-include tests.mk
