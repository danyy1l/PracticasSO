#Para que use la funcion time correctamente
SHELL := $(shell command -v zsh || command -v bash || echo /bin/sh)
# Hago esto en cascada porque prefiero la funcion time de zsh,
# que me da el porcentaje de CPU utilizado, si no se tuviera esta shell,
# pasaria a bash, y en caso critico a sh

EXE = mrush

.PHONY: test_hetero test_scaling test_network test_all clean_test_files

# Utilidad para limpiar archivos compartidos entre pruebas
clean_test_files:
	@-rm -f test_logs/*.log

# =======================================================================
# Redes Heterogéneas
# Demostrar que en la misma red, el minero con mas hilos gana mas
# =======================================================================
test_hetero: $(EXE)
	@mkdir -p test_logs
	@echo "\n======================================================="
	@echo " PRUEBA 1: Competición asimétrica (Distintos hilos en red)"
	@echo "=======================================================\n"
	@$(MAKE) clean_test_files
	@echo "\n---> 6 MINEROS: 10 vida | [Mineros con 2, 4, 8, 10, 14 y 16 Hilos]"
	@$(MAKE) clean_test_files
	@cd test_logs && time ( ../$(EXE) 10 2 & ../$(EXE) 10 4 & ../$(EXE) 10 8 & ../$(EXE) 10 10 & ../$(EXE) 10 14 & ../$(EXE) 10 16 & wait )
# =======================================================================
# Escalado de Rendimiento y Degradación
# Ver que a mas tiempo/hilos = mas rondas, pero pasarse colapsa el PC
# =======================================================================
test_scaling: $(EXE)
	@mkdir -p test_logs
	@echo "\n======================================================="
	@echo " PRUEBA 2: Escalado y Degradación (Red fija de 3 Mineros)"
	@echo "=======================================================\n"
	@$(MAKE) clean_test_files
	@echo "---> BASELINE: 5s de vida | 2 HILOS por minero"
	@cd test_logs && time ( ../$(EXE) 5 2 & ../$(EXE) 5 2 & ../$(EXE) 5 2 & wait )
	
	@echo "\n---> MAS TIEMPO (Mas rondas): 10s de vida | 2 HILOS por minero"
	@$(MAKE) clean_test_files
	@cd test_logs && time ( ../$(EXE) 10 2 & ../$(EXE) 10 2 & ../$(EXE) 10 2 & wait )
	
	@echo "\n---> MAS HILOS (Calculo mas rapido): 10s de vida | 8 HILOS por minero"
	@$(MAKE) clean_test_files
	@cd test_logs && time ( ../$(EXE) 10 8 & ../$(EXE) 10 8 & ../$(EXE) 10 8 & wait )
	
	@echo "\n---> DEGRADACION (Sobrecarga CPU): 10s de vida | 128 HILOS por minero (Rendimiento caerá)"
	@$(MAKE) clean_test_files
	@cd test_logs && time ( ../$(EXE) 10 128 & ../$(EXE) 10 128 & ../$(EXE) 10 128 & wait )

# =======================================================================
# Contención de Red (Overhead de IPC y Semáforos)
# Ver cómo el "System Time" sube al escalar procesos de 2 a 10
# =======================================================================
test_network: $(EXE)
	@mkdir -p test_logs
	@echo "\n======================================================="
	@echo " PRUEBA 3: Estrés de Red (IPC Overhead)"
	@echo " Fijos: 5s de vida | 4 HILOS por minero"
	@echo "=======================================================\n"
	@$(MAKE) clean_test_files
	@echo "---> Red de 2 MINEROS simultáneos"
	@cd test_logs && time ( ../$(EXE) 5 4 & ../$(EXE) 5 4 & wait )
	
	@echo "\n---> Red de 3 MINEROS simultáneos"
	@$(MAKE) clean_test_files
	@cd test_logs && time ( ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & wait )
	
	@echo "\n---> Red de 5 MINEROS simultáneos"
	@$(MAKE) clean_test_files
	@cd test_logs && time ( ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & wait )
	
	@echo "\n---> Red de 10 MINEROS simultáneos (Colisión de semáforos)"
	@$(MAKE) clean_test_files
	@cd test_logs && time ( ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & wait )
	
	@echo "\n---> Red de 20 MINEROS simultáneos (Alta colisión de semáforos)"
	@$(MAKE) clean_test_files
	@cd test_logs && time ( ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & ../$(EXE) 5 4 & wait )

test_all: test_hetero test_scaling test_network
	@echo "\n=== TODAS LAS PRUEBAS DE RENDIMIENTO COMPLETADAS ==="
