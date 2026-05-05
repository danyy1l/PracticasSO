/**
 * @file monitor_core.c
 * @author Fernando Blanco
 * @brief Implementación de la lógica del monitor y comprobador.
 *
 * @version 1.0
 */
#include "monitor_core.h"
#include "pow.h"
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Definición de la variable global de control
volatile sig_atomic_t stop_monitor = 0;

static void handler(int sig) {
  if (sig == SIGINT)
    stop_monitor = 1;
}

void setup_interrupt(void) {
  struct sigaction act;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  act.sa_handler = handler;

  if (sigaction(SIGINT, &act, NULL) == ERR) {
    perror("sigaction SIGINT");
    exit(EXIT_FAILURE);
  }
}

void comprobador(u64 lag_comprobador, SharedMinerData *miner_shm,
                 SharedMonitorData *monitor_shm, mqd_t miner_mq) {
  MinerDataBlock msg_recibido;
  bool is_end_signal = false;

  while (!stop_monitor && !is_end_signal) {
    ssize_t bytes = mq_receive(miner_mq, (char *)&msg_recibido,
                               sizeof(MinerDataBlock), NULL);

    if (bytes == ERR) {
      if (errno == EINTR)
        continue;
      perror("mq_receive");
      break;
    }

    if (msg_recibido.target == (u64)-1)
      is_end_signal = true;

    bool accepted = false;
    if (!is_end_signal) {
      accepted = (pow_hash(msg_recibido.solution) == msg_recibido.target);

      if (accepted) {
        sem_wait(&miner_shm->mutex);
        for (int i = 0; i < MAX_MINERS; i++) {
          if (miner_shm->miners[i].miner_pid == msg_recibido.miner_pid) {
            miner_shm->miners[i].coins++;
            break;
          }
        }
        sem_post(&miner_shm->mutex);
      }
    }

    /* Logica Productor */
    while (sem_wait(&monitor_shm->empty) == ERR) {
      if (errno == EINTR && stop_monitor)
        return;
    }
    if (stop_monitor) {
      sem_post(&monitor_shm->empty);
      return;
    }
    while (sem_wait(&monitor_shm->mutex) == ERR) {
      if (errno == EINTR && stop_monitor) {
        sem_post(&monitor_shm->empty);
        return;
      }
    }

    u64 idx = monitor_shm->write_idx;
    monitor_shm->data[idx].data = msg_recibido;
    monitor_shm->data[idx].accepted = accepted;
    monitor_shm->write_idx = (idx + 1) % MAX_MESSAGE_MONITOR;

    sem_post(&monitor_shm->mutex);
    sem_post(&monitor_shm->fill);

    usleep(lag_comprobador * 1000);
  }
}

void monitor(u64 lag_monitor, SharedMonitorData *monitor_shm) {
  MonitorDataBlock msg_monitor;
  bool is_end_signal = false;

  while (!stop_monitor && !is_end_signal) {

    /* Logica consumidor */
    while (sem_wait(&monitor_shm->fill) == ERR) {
      if (errno == EINTR && stop_monitor)
        return;
    }

    while (sem_wait(&monitor_shm->mutex) == ERR) {
      if (errno == EINTR && stop_monitor)
        return;
    }

    u64 idx = monitor_shm->read_idx;
    msg_monitor = monitor_shm->data[idx];
    monitor_shm->read_idx = (idx + 1) % MAX_MESSAGE_MONITOR;

    sem_post(&monitor_shm->mutex);
    sem_post(&monitor_shm->empty);

    if (msg_monitor.data.target == (u64)-1) {
      is_end_signal = true;
    } else {
      if (msg_monitor.accepted) {
        printf("Solution accepted: %08lu --> %08lu\n", msg_monitor.data.target,
               msg_monitor.data.solution);
      } else {
        printf("Solution rejected: %08lu !-> %08lu\n", msg_monitor.data.target,
               msg_monitor.data.solution);
      }
    }

    usleep(lag_monitor * 1000);
  }
}
