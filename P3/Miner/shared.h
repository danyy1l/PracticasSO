/**
 * @file shared.h
 * @author Fernando Blanco & Danyyil Shykerynets
 * @brief Interfaz de memoria compartida.
 * * Contiene las declaración de funciones y estructuras necesarios para el
 * manejo de la memoria compartida entre minero y monitor
 * @version 1.0
 * @date 2026-04-01
 */

#ifndef _SHARED_H
#define _SHARED_H

#include "types.h"
#include <mqueue.h>
#include <semaphore.h>

#define MINER_SHM "/miner_data"
#define MONITOR_SHM "/monitor_data"

#define MINER_MQ "/miner_queue"

#define MAX_MESSAGE_MINER 7
#define MAX_MESSAGE_MONITOR 6

/**
 * @struct MinerNode
 * @brief Representa el estado y la información de un minero individual en la
 * red
 *
 * Unifica los datos del minero para evitar arrays paralelos
 * Un nodo se considera inactivo si su campo pid es igual a 0
 */
typedef struct {
  u64 coins;         /**< Total de monedas del minero */
  pid_t miner_pid;   /**< PID del minero. Si es 0, inactivo */
  char current_vote; /**< Voto actual del resultado ganador ('Y', 'N', '\0') */
} MinerNode;
// Datos pequeños al final, evita padding intermedio. 16 bytes

/**
 * @struct SharedMinerData
 * @brief Estructura principal de memoria compartida para la red de mineros
 *
 * Sustituye a los ficheros temporales. Centraliza el objetivo actual,
 * la cuenta de mineros y contiene el array de nodos (mineros) de la red
 */
typedef struct {
  MinerNode miners[MAX_MINERS]; /**< Lista de mineros (4kB) */
  sem_t mutex;                  /**< Protección de la estructura */
  sem_t win;                    /**< Proteccion del ganador */
  u64 miner_target;             /**< Objetivo de la ronda */
  u64 active_miners;            /**< Numero de mineros activos */
} SharedMinerData;

/**
 * @struct MinerDataBlock
 * @brief Mensaje enviado por el minero ganador al comprobador a través de la
 * cola de mensajes
 *
 * Contiene los datos crudos de la ronda resuelta. Para indicar la finalización
 * del sistema, se enviará un bloque con target = (u64)-1
 */
typedef struct {
  u64 target;      /**< Target de la ronda. (u64)-1 indica finalizar la red */
  u64 solution;    /**< Solucion ganadora de la ronda */
  u64 round;       /**< Numero de ronda de busqueda */
  pid_t miner_pid; /**< PID del minero ganador */
} MinerDataBlock;

/**
 * @struct MonitorDataBlock
 * @brief Bloque de información verificado por comprobador y destinado al
 * monitor
 *
 * Utiliza composición: incrusta el mensaje original del minero y añade
 * el resultado de la validacion del Comprobador.
 */
typedef struct {
  MinerDataBlock data; /**< Mensaje original del minero ganador */
  bool accepted;       /**< Validacion del comprobador */
} MonitorDataBlock;

/**
 * @struct SharedMonitorData
 * @brief Memoria compartida para el esquema Productor-Consumidor (Comprobador
 * -> Monitor)
 *
 * Implementa un búfer circular de tamaño fijo y aloja los semáforos
 * anónimos necesarios para la sincronización
 */
typedef struct {
  sem_t mutex;   /**< Mutex de acceso al buffer */
  sem_t empty;   /**< Mutex de recuento de huecos libres */
  sem_t fill;    /**< Mutex de recuento de mensajes listos para leerse */
  u64 write_idx; /**< Indice de escritura (rear). Necesario para Producer */
  u64 read_idx;  /**< Indice de lectura (front). Necesario para COnsumer */
  MonitorDataBlock
      data[MAX_MESSAGE_MONITOR]; /**< Buffer circular que almacena mensajes */
} SharedMonitorData;

/**
 * @brief Crea la memoria compartida de mineros
 * @return Puntero a estructura SharedMinerData
 */
SharedMinerData *create_miner_shm();

/**
 * @brief Se une a la memoria compartida de mineros
 * @return Puntero a estructura SharedMinerData (no la crea)
 */
SharedMinerData *try_open_miner_shm();

/**
 * @brief Crea la memoria compartida entre monitor y minero ganador
 * @return Puntero a estructura SharedMonitorData
 */
SharedMonitorData *create_monitor_shm();

/**
 * @brief Se une a la memoria compartida de monitor
 * @return Puntero a estructura SharedMonitorData (no la crea)
 */
SharedMonitorData *try_open_monitor_shm();

/**
 * @brief Crea una cola de mensajes
 * @return Descriptor de cola de mensajes mqd_t
 */
mqd_t create_miner_queue();

/**
 * @brief Intenta enlazarse a una cola de mensajes existente
 * @return Descriptor de cola de mensajes mqd_t
 */
mqd_t try_open_miner_queue();

/**
 * @brief Cierra y destruye la memoria compartida de mineros
 * @param shared Puntero a la memoria mapeada
 */
void destroy_miner_shm(SharedMinerData *shared);

/**
 * @brief Cierra y destruye la memoria compartida del monitor
 * @param shared Puntero a la memoria mapeada
 */
void destroy_monitor_shm(SharedMonitorData *shared);

/**
 * @brief Cierra y destruye la cola de mensajes
 * @param mq Descriptor de la cola de mensajes
 */
void destroy_miner_queue(mqd_t mq);

/**
 * @brief Solo desmapea la memoria para los mineros que salen, sin destruirla
 * @param shared Puntero a la memoria mapeada
 */
void detach_miner_shm(SharedMinerData *shared);

#endif
