#Para que use la funcion time correctamente
SHELL := $(shell command -v zsh || command -v bash || echo /bin/sh)
# Hago esto en cascada porque prefiero la funcion time de zsh,
# que me da el porcentaje de CPU utilizado, si no se tuviera esta shell,
# pasaria a bash, y en caso critico a sh

MONITOR = monitor
MINER = mrush

.PHONY: test_ideal test_bottleneck_comprobador test_bottleneck_monitor test_massive test_all clean_test_files

clean_test_files:
	@-rm -f test_logs/*.log

# =======================================================================
# Servidor instantáneo
# El servidor consume los bloques tan rápido como se generan.
# La cola de mensajes casi siempre estará vacía.
# =======================================================================
test_ideal:
	@mkdir -p test_logs
	@echo "\n======================================================="
	@echo " PRUEBA 1: Servidor sin latencia (0ms / 0ms)"
	@echo "=======================================================\n"
	@$(MAKE) clean_test_files
	@cd test_logs && time ( ../$(MONITOR) 0 0 & \
		../$(MINER) 5 4 & ../$(MINER) 5 4 & ../$(MINER) 5 4 & wait )

# =======================================================================
# Cuello de botella en la Cola de Mensajes (Comprobador Lento)
# El comprobador tarda 200ms por bloque. Los mineros generan mucho más 
# rápido, así que la 'mq' se llenará. Los mineros se quedarán bloqueados 
# en mq_send, bajando su uso de CPU y tardando más en cerrarse.
# =======================================================================
test_bottleneck_comprobador:
	@mkdir -p test_logs
	@echo "\n======================================================="
	@echo " PRUEBA 2: Cuello de botella en Recepción (200ms / 0ms)"
	@echo "=======================================================\n"
	@$(MAKE) clean_test_files
	@cd test_logs && time ( ../$(MONITOR) 200 0 & \
		../$(MINER) 5 4 & ../$(MINER) 5 4 & ../$(MINER) 5 4 & wait )

# =======================================================================
#  Cuello de botella en Búfer Circular (Monitor Lento)
# El Comprobador verifica rápido (0ms), pero el Monitor imprime lento (300ms).
# Esto llenará el búfer circular de Memoria Compartida. El Comprobador se
# quedará bloqueado en sem_wait(&empty) y eventualmente la 'mq' también 
# se llenará, bloqueando en cascada a los mineros.
# =======================================================================
test_bottleneck_monitor:
	@mkdir -p test_logs
	@echo "\n======================================================="
	@echo " PRUEBA 3: Cuello de botella en Impresión (0ms / 300ms)"
	@echo "=======================================================\n"
	@$(MAKE) clean_test_files
	@cd test_logs && time ( ../$(MONITOR) 0 300 & \
		../$(MINER) 5 4 & ../$(MINER) 5 4 & ../$(MINER) 5 4 & wait )

# =======================================================================
# Estrés Masivo de Red (Muchos clientes)
# Comprobamos cómo gestiona el SO y la memoria compartida la concurrencia
# de 20 mineros compitiendo por entrar, votar y encolar mensajes.
# =======================================================================
test_massive:
	@mkdir -p test_logs
	@echo "\n======================================================="
	@echo " PRUEBA 4: Estrés Masivo (20 Mineros vs 1 Servidor)"
	@echo "=======================================================\n"
	@$(MAKE) clean_test_files
	@cd test_logs && time ( ../$(MONITOR) 10 10 & \
		../$(MINER) 5 2 & ../$(MINER) 5 2 & ../$(MINER) 5 2 & ../$(MINER) 5 2 & ../$(MINER) 5 2 & \
		../$(MINER) 5 2 & ../$(MINER) 5 2 & ../$(MINER) 5 2 & ../$(MINER) 5 2 & ../$(MINER) 5 2 & \
		../$(MINER) 5 2 & ../$(MINER) 5 2 & ../$(MINER) 5 2 & ../$(MINER) 5 2 & ../$(MINER) 5 2 & \
		../$(MINER) 5 2 & ../$(MINER) 5 2 & ../$(MINER) 5 2 & ../$(MINER) 5 2 & ../$(MINER) 5 2 & wait )

test_all: test_ideal test_bottleneck_comprobador test_bottleneck_monitor test_massive
	@echo "\n=== TODAS LAS PRUEBAS DE ESTRÉS COMPLETADAS ==="
