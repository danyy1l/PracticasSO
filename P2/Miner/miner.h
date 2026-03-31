/**
 * @file miner.h
 * @author Fernando Blanco & Danyyil Shykerynets
 * @brief Interfaz de mineros.
 * * Contiene las declaración de funciones y estructuras necesarios para el
 * minero y las tuberias así como las funciones de cálculo
 * @version 1.0
 * @date 2026-02-12
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
  u64 target;                /**< Objetivo de busqueda de funcion hash */
  u64 min;                   /**< Valor minimo de busqueda */
  u64 max;                   /**< Valor maximo de busqueda */
  volatile int *found_value; /**< Booleano de hallazgo de la solucion */
} Arg_hilos;

/**
 * @brief Estructura para manejo de argumentos de Minero
 * Servira para guardar los datos relevantes para el minero
 */

typedef struct {
  u64 target;    /**< Objetivo de busqueda de funcion hash */
  u64 rounds;    /**< Numero maximo de rondas de busqueda */
  u64 n_threads; /**< Numero de hilos en los que separar carga de trabajo */
} Miner_data;

/* Al ser estatica hay que definirla en .h porque todos los archivos que la usen
 * necesitan una copia del codigo, si estuviera en .c (opaca) habria error
 * puesto que no podrian emplear la cualidad de inline */

/******************************* DATOS PRIVADOS ****************************/

/**
 * @brief Gestiona toda la lógica del minero
 * Esta funcion llama a create_threads y manda informacion a logger a traves de
 * pipes
 *
 * @param miner_data Estructura con informacion para minero (rondas, target y
 * numero de hilos)
 * @param miner_pipe Tuberia minero--->registrador
 * @param logger_pipe Tuberia registrador---->minero
 */
void minero(Miner_data *args, i32 *miner_pipe, i32 *logger_pipe);

/**
 * @brief Busca el valor objetivo en un rango dado
 * Funcion de calculo
 *
 * @param arg Puntero a lista de argumentos
 * @return Devuelve puntero al numero que consigue el valor objetivo
 */
void *pow_seek(void *arg);

/**
 * @brief Crea los hilos y separa la tarea
 *
 * @param miner_data Estructura con informacion para minero (rondas, target y
 * numero de hilos)
 * @return Devuelve la solucion para el POW con objetivo args->target
 */
u64 calcular_solucion(Miner_data *args);

/**
 * @brief Apertura de tuberias
 *
 * @param miner_pipe Tuberia minero--->registrador
 * @param logger_pipe Tuberia registrador---->minero
 * En caso de error termina el programa con die()
 */
void open_pipes(i32 *miner_pipe, i32 *logger_pipe);

/**
 * @brief Cierre de tuberias
 *
 * @param miner_pipe Tuberia minero--->registrador
 * @param logger_pipe Tuberia registrador---->minero
 */
void close_pipes(i32 *miner_pipe, i32 *logger_pipe);

#endif
