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

#include "files.h"
#include "types.h"
#include <pthread.h>
#include <semaphore.h>
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
  u64 time;      /**< Tiempo en segundos de ejecucion del minero */
  u64 n_threads; /**< Numero de hilos en los que separar carga de trabajo */
} Miner_data;

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
 * @param sems Estructura de semaforos del sistema
 */
void minero(Miner_data *args, i32 *miner_pipe, i32 *logger_pipe,
            Miner_Mutexes *sems);

/** @name Funciones expuestas para testing */
/**@{*/
void wait_more_miners(Miner_Mutexes *sems);
void wait_votes(u32 votes, Miner_Mutexes *sems);
u64 calcular_solucion(u64 target, Miner_data *args);
/**@}*/

#endif
