/**
 * @file utils.h
 * @author Danyyil Shykerynets
 * @brief Interfaz de utilidades
 * * Contiene las funciones utiles para el proyecto
 * @version 1.0
 * @date 2026-04-02
 *
 * @copyright (c) 2026 Author. All Rights Reserved.
 */

#ifndef _UTILS_H
#define _UTILS_H

#include "types.h"

/** @name Extremos de tuberías (Pipes) */
/**@{*/
#define READ 0  /**< Índice del descriptor de archivo para lectura */
#define WRITE 1 /**< Índice del descriptor de archivo para escritura */
/**@}*/

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

/**
 * Parsea un numero en string a unsigned int de 64 bit de forma segura
 *
 * @param input String a parsear
 * @return Unsigned int de 64 bit o ERR en su caso
 */
u64 str_to_u64(char *input);

#endif
