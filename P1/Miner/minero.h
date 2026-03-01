/**
 * @file miner.h
 * @author Fernando Blanco & Danyyil Shykerynets
 * @brief Manejo de mineros.
 * @version 1.0
 * @date 2026-02-17
 *
 */

#ifndef _MINER_H
#define _MINER_H

#include "types.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/******************************* DATOS PUBLICOS ****************************/

/**
 * @brief Estructura para manejo de argumentos de hilos
 * Servira para guardar valores para busqueda de hash a traves de POW
 */
typedef struct {
  u64 target;                // Objetivo de busqueda de funcion hash
  u64 min;                   // Valor minimo de busqueda
  u64 max;                   // Valor maximo de busqueda
  volatile int *found_value; // Puntero para que cuando un hilo modifique, todos
                             // revisen misma dir de memoria
} Arg_hilos;

/**
 * @brief Imprimir error y terminar el programa
 *
 * @param msg Cadena con mensaje de error
 */
static inline void die(char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

/* Al ser estatica hay que definirla en .h porque todos los archivos que la usen
 * necesitan una copia del codigo, si estuviera en .c (opaca) habria error
 * puesto que no podrian emplear la cualidad de inline */

/******************************* DATOS PRIVADOS ****************************/

/**
 * @brief Gestiona toda la lógica del minero
 * Esta funcion llama a create_threads y manda informacion a logger a traves de
 * pipes
 *
 * @param target Objetivo de busqueda con minero
 * @param rounds Maximo de rondas totales de busqueda
 * @param n_threads Numero de hilos en los que dividir trabajo
 */
void minero(i64 target, u64 rounds, u64 n_threads);

/**
 * @brief Busca el valor objetivo en un rango dado
 * Funcion de calculo
 *
 * @param arg Puntero a lista de argumentos
 * @return Devuelve el numero que consigue el valor objetivo
 */
void *pow_seek(void *arg);

/**
 * @brief Crea los hilos y separa la tarea
 *
 * @param target Objetivo de busqueda
 * @param rounds Numero maximo de rondas
 * @param n_threads Numero de hilos en los que separar
 */
u64 create_threads(i64 target, u64 rounds, u64 n_threads);

/**
 * @brief Apertura de tuberias
 *
 * @param miner_pipe Tuberia minero--->registrador
 * @param logger_pipe Tuberia registrador---->minero
 * En caso de error termina el programa con die()
 */
void open_pipes(i32 *miner_pipe, i32 *logger_pipe);

/**
 * @brief Cerrado de pipes
 *
 * @param miner_pipe Tuberia minero--->registrador
 * @param logger_pipe Tuberia registrador---->minero
 */
void close_pipes(i32 *miner_pipe, i32 *logger_pipe);

#endif
