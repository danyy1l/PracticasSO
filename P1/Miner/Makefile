EXE=miner
CC=cc
CFLAGS=-Wall -Wextra -std=gnu99 -pedantic -g
CLIBS=-lpthread

OBJ_DIR = obj

.PHONY: clean run

HEADERS=minero.h pow.h logger.h types.h
OBJ= $(addprefix $(OBJ_DIR)/, pow.o logger.o minero.o $(EXE).o)

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(CLIBS)

$(OBJ_DIR)/%.o : %.c $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(EXE)

run: all
	./$(EXE) 1 2 3
