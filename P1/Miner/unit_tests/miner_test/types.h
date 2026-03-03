/**
 * @file types.h
 * @author Danyyil Shykerynets
 * @brief Definición de tipos de datos globales
 * * Este archivo contiene los alias de tipos de tamaño fijo (stdint)
 * y las estructuras fundamentales compartidas entre el minero y el logger.
 * @version 1.0
 * @date 2026-02-09
 */

#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>
#include <sys/types.h>

/** @name Tipos de datos enteros y flotantes
 **@{*/
typedef int32_t i32;  /**< Entero con signo 32 bit. [-2^31, 2^31 - 1] */
typedef int64_t i64;  /**< Entero con signo 64 bit. [-2^63, 2^63 - 1] */
typedef uint32_t u32; /**< Entero sin signo 32 bit. [0, 2^32 - 1] */
typedef uint64_t u64; /**< Entero sin signo 64 bit. [0, 2^64 - 1] */
typedef float f32;    /**< Float 32 bit. 7 dig. precision */
typedef double f64;   /**< Float 64 bit. 15 dig. precision */
/**@}*/

/** @name Control de errores */
/**@{*/
#define ERR -1    /**< Código estándar de error para returns */
#define OK !(ERR) /**< Código de éxito (0) */

/**
 * @brief Imprimir error y terminar el programa
 *
 * @param msg Cadena con mensaje de error
 */
#define die(msg)                                                               \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)
/**@}*/

/** @name Extremos de tuberías (Pipes) */
/**@{*/
#define READ 0  /**< Índice del descriptor de archivo para lectura */
#define WRITE 1 /**< Índice del descriptor de archivo para escritura */
/**@}*/

/** @name Banderas de estado */
/**@{*/
#define FOUND 1 /**< Indica que un hilo ha encontrado solución */
/**@}*/

/** @name Utilidades generales */
/**@{*/
/**
 * @brief Devuelve el valor mínimo entre dos elementos.
 * @param a Primer valor a comparar.
 * @param b Segundo valor a comparar.
 * @return El menor de los dos valores.
 */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
/**@}*/

#endif
