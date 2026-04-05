/**
 * @file files.h
 * @author Danyyil Shykerynets
 * @brief Interfaz de manejo de archivos.
 * * Contiene las funciones necesarias para lectura y escritura en los ficheros
 * del proyecto.
 * @version 1.0
 * @date 2026-04-02
 *
 * @copyright (c) 2026 Author. All Rights Reserved.
 */

#ifndef _FILES_H
#define _FILES_H

#include "types.h"
#include <semaphore.h>

/** @name Extremos de tuberías (Pipes) */
/**@{*/
#define READ 0  /**< Índice del descriptor de archivo para lectura */
#define WRITE 1 /**< Índice del descriptor de archivo para escritura */
/**@}*/

/** @name Nombres de ficheros */
/**@{*/
#define TARGET_FILE "Target.tgt" /**< Fichero con los objetivos del minero */
#define PID_FILE "PIDs.pid"      /**< Fichero con los PIDs de los mineros */
#define VOTES_FILE "Voting.vot"  /**< Fichero con registro de votos */
/**@}*/

/** @name Semaforos de ficheros */
/**@{*/
#define TARGET_MUTEX "/sem_miner_target" /**< Mutex de fichero de objetivo */
#define PID_MUTEX "/sem_miner_pid" /**< Mutex de fichero de pids de mineros */
#define VOTES_MUTEX "/sem_miner_votes"   /**< Mutex de fichero de votos */
#define WINNER_MUTEX "/sem_miner_winner" /**< Mutex para ganador de ronda */
/**@}*/

/**
 * @brief Define el struct de mutexes que se usan en el proyecto
 */
typedef struct {
  sem_t *pid; /**< Mutex que protege el fichero de PIDs */
  sem_t *tgt; /**< Mutex que protege el fichero de target */
  sem_t *vot; /**< Mutex que protege el fichero de votos */
  sem_t *win; /**< Mutex que protege las secuencias del ganador de ronda */
} Miner_Mutexes;

/**
 * @brief Inicializa los semaforos a 1
 * Si no existian, los crea, en caso contrario, obtiene la direccion de los
 * semaforos ya creados
 *
 * @return Abre los semaforos de una estructura Miner_Mutexes
 */
void initialize_mutexes(Miner_Mutexes *sems);

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
 * @brief Apunta el pid en el fichero de PIDs
 *
 * @param filename Nombre del fichero
 * @param mutex_pid Mutex que protege el fichero
 * @return OK en caso de EXITO, ERR si algo ha salido mal
 */
i32 write_pid_unlocked(const char *filename);

/**
 * @brief Lee el objetivo desde el fichero
 *
 * @param filename Nombre del fichero con TARGET
 * @param out_target Puntero de valor de retorno (Target)
 * @return OK en caso de exito, ERR en caso contrario
 */
i32 read_target_unlocked(const char *filename, u64 *out_target);

/**
 * @brief Escribe un nuevo objetivo en el fichero
 *
 * @param filename Nombre del fichero con TARGET
 * @param target Valor numérico del objetivo a escribir
 * @return OK en caso de exito, ERR en caso de fallo
 */
i32 write_target_unlocked(const char *filename, u64 target);

/**
 * @brief Obtiene los pids activos y limpia los inactivos del fichero
 *
 * @param filename Nombre del fichero con PIDS
 * @param active_miners Puntero a array donde guardar los pids activos (No
 * reserva memoria)
 * @param removed_pid PID del proceso a eliminar de los activos (en general
 * getpid())
 * @param cleanup Boolean para eliminar removed_pid si TRUE
 * @return Devuelve el numero de procesos activos del fichero o ERR en caso de
 * fallo
 */
i32 get_active_pids_unlocked(const char *filename, pid_t *active_miners,
                             pid_t removed_pid, bool cleanup);

/**
 * @brief Escribe el voto del proceso en el fichero
 *
 * @param filename Nombre del fichero con votos
 * @param vote Char con voto (Y or N)
 */
void write_vote(const char *filename, char vote);

/**
 * @brief Hace el recuento de votos de la solucion
 *
 * @param filename Nombre de fichero con votos del resto
 * @param winner_pid PID del proceso ganador de la ronda
 * @param out_positives Puntero en el que se guardan el numero de positivos
 * @return TRUE si la solucion ha sido aceptada, FALSE en caso contrario
 */
bool count_votes(const char *filename, pid_t winner_pid, u32 *out_positives);

#endif
