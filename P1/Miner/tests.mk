#Para que use la funcion time correctamente
SHELL := $(shell command -v zsh || command -v bash || echo /bin/sh)
# Hago esto en cascada porque prefiero la funcion time de zsh,
# que me da el porcentaje de CPU utilizado, si no se tuviera esta shell,
# pasaria a bash, y en caso critico a sh

TARGET_INI = 0
ROUNDS_FIJAS = 20
THREADS_FIJOS = 4

.PHONY: test_threads test_rounds test_all

# Prueba variando hilos
test_threads: $(EXE)
	@mkdir -p test_logs
	@echo ""
	@echo "======================================================="
	@echo " PRUEBA 1: Variando número de hilos (Rondas fijas: $(ROUNDS_FIJAS))"
	@echo "======================================================="
	@echo ""
	@echo "---> Ejecutando con 1 HILO"
	@cd test_logs && time ../$(EXE) $(TARGET_INI) $(ROUNDS_FIJAS) 1
	@echo ""
	@echo "---> Ejecutando con 2 HILOS"
	@cd test_logs && time ../$(EXE) $(TARGET_INI) $(ROUNDS_FIJAS) 2
	@echo ""
	@echo "---> Ejecutando con 4 HILOS"
	@cd test_logs && time ../$(EXE) $(TARGET_INI) $(ROUNDS_FIJAS) 4
	@echo ""
	@echo "---> Ejecutando con 8 HILOS"
	@cd test_logs && time ../$(EXE) $(TARGET_INI) $(ROUNDS_FIJAS) 8
	@echo ""
	@echo "---> Ejecutando con 16 HILOS"
	@cd test_logs && time ../$(EXE) $(TARGET_INI) $(ROUNDS_FIJAS) 16
	@echo ""
	@echo "---> Ejecutando con 32 HILOS"
	@cd test_logs && time ../$(EXE) $(TARGET_INI) $(ROUNDS_FIJAS) 32
	@echo ""
	@echo "---> Ejecutando con 64 HILOS"
	@cd test_logs && time ../$(EXE) $(TARGET_INI) $(ROUNDS_FIJAS) 64

# Prueba variando rondas
test_rounds: $(EXE)
	@mkdir -p test_logs
	@echo ""
	@echo "======================================================="
	@echo " PRUEBA 2: Variando número de rondas (Hilos fijos: $(THREADS_FIJOS))"
	@echo "======================================================="
	@echo ""
	@echo "---> Ejecutando con 10 RONDAS"
	@cd test_logs && time ../$(EXE) $(TARGET_INI) 10 $(THREADS_FIJOS)
	@echo ""
	@echo "---> Ejecutando con 50 RONDAS"
	@cd test_logs && time ../$(EXE) $(TARGET_INI) 50 $(THREADS_FIJOS)
	@echo ""
	@echo "---> Ejecutando con 100 RONDAS"
	@cd test_logs && time ../$(EXE) $(TARGET_INI) 100 $(THREADS_FIJOS)
	@echo ""


test_all: test_threads test_rounds
	@echo ""
	@echo "=== TODAS LAS PRUEBAS DE RENDIMIENTO COMPLETADAS ==="
