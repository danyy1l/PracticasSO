/**
 * @file shared.h
 * @author Fernando Blanco & Danyyil Shykerynets
 * @brief Interfaz de memoria compartida.
 * * Contiene las declaración de funciones y estructuras necesarios para el manejo
 * de la memoria compartida entre minero y monitor
 * @version 1.0
 * @date 2026-04-01
 *
 */

#ifndef _SHARED_H
#define _SHARED_H

#include "types.h"
#include <semaphore.h>

#define MINER_SHM "/miner_data"
#define MONITOR_SHM "/monitor_data"

#define MINER_MQ "/miner_queue"
#define MONITOR_MQ "/monitor_queue"

#define MAX_MESSAGE_MINER 7
#define MAX_MESSAGE_MONITOR 6


typedef struct {
  sem_t mutex;
  pid_t miner_pids[MAX_MINERS];
  u64 miner_count;
  u64 miner_target;
  char votes[MAX_MINERS];
  u64 active_miners;
} SharedMinerData;

typedef struct {
    sem_t mutex;
    sem_t empty;
    sem_t fill;
    MonitorDataBlock data;
} SharedMonitorData;

typedef struct {
    /*
    u64 target;
    u64 winner;
    u64 round;
    char winner_pid[20];
    */
} MonitorDataBlock;

typedef struct {
    /*
    u64 target;
    u64 winner;
    u64 round;
    char winner_pid[20];
    */
} MinerDataBlock;

SharedMinerData *create_miner_shm();

SharedMinerData *try_open_miner();



#endif