#!/bin/bash

# Verificar argumentos
if [ "$#" -lt 1 ]; then
  echo "Error: Uso: $0 <0|1> [nombre_directorio_test]"
  exit 1
fi

modo=$1
test_dir=$2 # Segundo argumento (opcional)

# Validar el modo de ejecución
if [ "$modo" -ne 0 ] && [ "$modo" -ne 1 ]; then
  echo "Error: El primer argumento debe ser 0 o 1"
  exit 1
fi

function procesar_test() {
  dir="$1"
  echo "Entrando en el directorio: $dir"
  cd "$dir" || {
    echo "Error: No se pudo acceder a $dir"
    exit 1
  }

  # Compilar y ejecutar
  echo "Compilando en $dir..."
  make clean >/dev/null # Limpiar antes de compilar (opcional)
  make
  if [ $? -ne 0 ]; then
    echo "Error: Fallo al compilar en $dir"
    exit 1
  fi

  # Ejecución según el modo
  nombre_test=$(basename "$dir")
  if [ "$modo" -eq 0 ]; then
    echo "Ejecutando $nombre_test en modo normal..."
    ./"$nombre_test"
  elif [ "$modo" -eq 1 ]; then
    echo "Ejecutando $nombre_test con Valgrind..."
    valgrind --leak-check=full --track-origins=yes ./"$nombre_test"
  fi

  # Limpiar archivos .o después de ejecutar
  echo "Limpiando $dir..."
  make clean >/dev/null
  if [ $? -ne 0 ]; then
    echo "Advertencia: Fallo al limpiar $dir"
  fi

  cd .. # Volver al directorio raíz
}

# Lógica principal
if [ -z "$test_dir" ]; then # Si no hay segundo argumento
  echo "Ejecutando TODOS los tests..."
  for dir in *_test; do # Busca directorios que terminen en "-test"
    procesar_test "$dir"
  done
else # Si se indica un directorio específico
  if [ -d "$test_dir" ]; then
    procesar_test "$test_dir"
  else
    echo "Error: El directorio $test_dir no existe"
    exit 1
  fi
fi
