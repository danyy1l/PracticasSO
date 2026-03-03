EXE=mrush
CC=cc
CFLAGS=-Wall -Wextra -std=gnu99 -pedantic -g
CLIBS=-lpthread

OBJ_DIR = obj

.PHONY: clean run doc test_threads test_rounds test_all

HEADERS=miner.h pow.h logger.h types.h
OBJ= $(addprefix $(OBJ_DIR)/, pow.o logger.o miner.o $(EXE).o)

######### COMPILACION DE CODIGO ########

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(CLIBS)

$(OBJ_DIR)/%.o : %.c $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

######### INSTRUCCIONES ESPECIALES ########

#Borrar ficheros compilados, logs, documentacion y ejecutable
clean:
	-rm -rf *.log
	rm -rf DocumentacionMinero.pdf doc $(OBJ_DIR) $(EXE) test_logs

# Ejecutable base target 0, 5 rondas, 3 hilos
run: all
	./$(EXE) 0 5 3

# Ejecutar con valgrind
valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes --log-file=memcheck.log ./$(EXE) 1 10 8

# Ejecutar con prueba de sincronizacion de hilos
helgrind: all
	valgrind --tool=helgrind --log-file=thread_check.log ./mrush 1 5 4

# Generar documentacion
doc:
	doxygen Doxyfile
	$(MAKE) -C doc/latex
	cp doc/latex/refman.pdf ./DocumentacionMinero.pdf

# Makefile de tests
-include tests.mk
