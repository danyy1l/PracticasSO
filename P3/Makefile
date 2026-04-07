EXE=mrush
CC=cc
CFLAGS=-Wall -Wextra -O3 -std=gnu99 -pedantic -g
CLIBS=-lpthread -lrt # Libreria de tiempo real
FILES=$(wildcard *.log *.tgt *.pid *.vot)
VALG=valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes --log-file=memcheck-%p.log

OBJ_DIR = obj

.PHONY: clean run doc test_threads test_rounds test_all

HEADERS=miner.h pow.h logger.h types.h files.h
OBJ= $(addprefix $(OBJ_DIR)/, pow.o files.o logger.o miner.o $(EXE).o)

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
	-rm -rf $(FILES)
	rm -rf DocumentacionMinero.pdf doc $(OBJ_DIR) $(EXE) test_logs

# Ejecutable base 5 segundos, 5 hilos | 6 seg 4 hilos | 2 seg 10 hilos
run: all
	./$(EXE) 3 8 & ./$(EXE) 5 8 & ./$(EXE) 10 8 & ./$(EXE) 10 1 & wait

#Arreglo para que no quede en stall la terminal
#No es cosa del programa al parecer espera a que presione Enter, pero el codigo funciona

# Ejecutar con valgrind
valgrind: all
	$(VALG) ./$(EXE) 3 8 & $(VALG) ./$(EXE) 5 8 & $(VALG) ./$(EXE) 10 8 & $(VALG) ./$(EXE) 10 4 & wait

# Ejecutar con prueba de sincronizacion de hilos
helgrind: all
	valgrind --tool=helgrind --log-file=thread_check.log ./mrush 5 5 & ./mrush 7 10

# Generar documentacion
doc:
	doxygen Doxyfile
	$(MAKE) -C doc/latex
	cp doc/latex/refman.pdf ./DocumentacionMinero.pdf

fake: CFLAGS += -DFAKE
fake:
	$(MAKE) clean
	$(MAKE) all CFLAGS="$(CFLAGS)"

run_fake: fake
	./$(EXE) 3 8 & ./$(EXE) 5 8 & ./$(EXE) 10 8 & ./$(EXE) 10 1 & wait

# Makefile de tests
-include tests.mk
