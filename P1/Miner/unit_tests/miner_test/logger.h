/**
 * @file logger.h
 * @author Fernando Blanco & Danyyil Shykerynets
 * @brief Interfaz de registrador.
 * * Contiene las funciones necesarias para inicializar el registrador y
 * escribir el registro de soluciones en el archivo
 * @version 1.0
 * @date 2026-02-25
 *
 * @copyright (c) 2026 Author. All Rights Reserved.
 */

#ifndef _LOGGER_H
#define _LOGGER_H

#include "types.h"

/**
 * @brief Argumentos y datos del bloque minado
 * Empleamos esta estructura para manejar los argumentos del registrador
 * Son los argumentos que almacenara el registrador en el fichero
 */
typedef struct {
  u64 id;        /**< Numero de ronda */
  u64 target;    /**< Objetivo de la ronda */
  u64 solution;  /**< Valor que resuelve el POW */
  u64 votes;     /**< Numero de ronda */
  u64 wallets;   /**< En principio, id padre:numero ronda */
  pid_t winner;  /**< ID Proceso padre */
  i32 validated; /**< Booleano que representa validez de la solucion */
} Logger_args;

/* Dejamos pid_t(4bytes) y validated(4 Byte) para el final para minimizar
 * padding y alinear struct a 48 bytes. Podriamos usar tipos de menor tamaño
 * para validated, pero con 4 bytes conseguimos 0 bytes de padding */

/**
 * @brief Registra los datos del minero en un fichero
 * * Se espera a leer informacion del minero a traves de miner_pipe. A
 * continuación se escribe la informacion en el fichero con formato indicado en
 * el enunciado. Por último se indica al minero que continue buscando la
 * solucion de la siguiente ronda
 *
 * @param miner_pipe Tuberia de escucha del minero
 * @param logger_pipe Tuberia de escritura AL minero
 * @return Devuelve EXIT_SUCCESS en caso de exito, EXIT_FAILURE en caso
 * contrario
 */
i32 logger(i32 *miner_pipe, i32 *logger_pipe);

#endif
