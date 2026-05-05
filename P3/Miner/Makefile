EXE_MINER=mrush
EXE_MONITOR=monitor
CC=cc
CFLAGS=-Wall -Wextra -O3 -std=gnu99 -pedantic -g
CLIBS=-lpthread -lrt # Libreria de tiempo real
VALG=valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes --log-file=memcheck-%p.log

OBJ_DIR = obj

.PHONY: clean run doc test_threads test_rounds test_all valgrind helgrind

HEADERS=miner.h pow.h logger.h types.h utils.h shared.h monitor_core.h
#
# Objetos para el Minero
OBJ_MINER_FILES = pow.o utils.o logger.o miner.o shared.o mrush.o
OBJ_MINER = $(addprefix $(OBJ_DIR)/, $(OBJ_MINER_FILES))

# Objetos para el Monitor
OBJ_MONITOR_FILES = pow.o utils.o shared.o monitor_core.o monitor.o
OBJ_MONITOR = $(addprefix $(OBJ_DIR)/, $(OBJ_MONITOR_FILES))


######### COMPILACION DE CODIGO ########

all: $(EXE_MINER) $(EXE_MONITOR)

$(EXE_MINER): $(OBJ_MINER)
	$(CC) -o $@ $^ $(CLIBS)

$(EXE_MONITOR): $(OBJ_MONITOR)
	$(CC) -o $@ $^ $(CLIBS)

$(OBJ_DIR)/%.o : %.c $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

######### INSTRUCCIONES ESPECIALES ########

#Borrar ficheros compilados, logs, documentacion y ejecutable
clean:
	-rm -rf *.log
	rm -rf DocumentacionMinero.pdf doc $(OBJ_DIR) $(EXE_MINER) $(EXE_MONITOR) test_logs

# Ejecucion completa: Arranca monitor, espera 1s para que inicialice SHM, arranca mineros
run: all
	./$(EXE_MONITOR) 10 500 & \
	sleep 1; \
	./$(EXE_MINER) 3 8 & \
	./$(EXE_MINER) 5 8 & \
	./$(EXE_MINER) 10 8 & \
	./$(EXE_MINER) 10 1 & \
	wait

# Ejecutar con valgrind
valgrind: all
	./$(EXE_MONITOR) 10 500 & \
	sleep 1; \
	$(VALG) ./$(EXE_MINER) 3 8 & \
	$(VALG) ./$(EXE_MINER) 5 8 & \
	$(VALG) ./$(EXE_MINER) 10 8 & \
	$(VALG) ./$(EXE_MINER) 10 4 & \
	wait

# Ejecutar con prueba de sincronizacion de hilos
helgrind: all
	./$(EXE_MONITOR) 10 500 & \
	sleep 1; \
	valgrind --tool=helgrind --log-file=thread_check.log ./$(EXE_MINER) 5 5 & \
	./$(EXE_MINER) 7 10 & \
	wait

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
	./$(EXE_MONITOR) 10 500 & \
	sleep 1; \
	./$(EXE_MINER) 3 8 & \
	./$(EXE_MINER) 5 8 & \
	./$(EXE_MINER) 10 8 & \
	./$(EXE_MINER) 7 1 & \
	wait

# Makefile de tests
-include tests.mk
