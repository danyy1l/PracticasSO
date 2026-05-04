/**
 * @file shared.h
 * @author Fernando Blanco & Danyyil Shykerynets
 * @brief Interfaz de memoria compartida.
 * * Contiene las declaración de funciones y estructuras necesarios para el
 * manejo de la memoria compartida entre minero y monitor
 * @version 1.0
 * @date 2026-04-01
 *
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

typedef struct {
  pid_t miner_pid;
  u64 coins;
} MinerWallet;

typedef struct {
  u64 target;
  u64 solution;
  u64 round;
  pid_t miner_pid;
  bool is_end;
} MinerDataBlock;

typedef struct {
  sem_t mutex;
  pid_t miner_pids[MAX_MINERS];
  u64 miner_count;
  u64 miner_target;
  MinerWallet miner_wallets[MAX_MINERS];
  char votes[MAX_MINERS];
  u64 active_miners;
} SharedMinerData;

typedef struct {
  u64 target;
  u64 solution;
  u64 round;
  pid_t miner_pid; /* PID del minero "ganador"*/
  bool accepted;   /* Si la solucion es valida o no*/
  bool is_end;     /* Si es la ultima solucion del round */
} MonitorDataBlock;

typedef struct {
  sem_t mutex;
  sem_t empty;
  sem_t fill;
  u64 write_idx;
  u64 read_idx;
  MonitorDataBlock data;
} SharedMonitorData;

SharedMinerData *create_miner_shm();

SharedMinerData *try_open_miner_shm();

SharedMonitorData *create_monitor_shm();

SharedMonitorData *try_open_monitor_shm();

mqd_t create_miner_queue();

mqd_t try_open_miner_queue();

#endif
