/**
 * @file shared.c
 * @author Fernando Blanco
 * @brief Implementa las utilidades de memoria compartida
 *
 * @version 1.0
 * @Copyright (c) 2026 Author. All Rights Reserved.
 */

#include "shared.h"
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/* ************************************** */
/* ** FUNCIONES DE CREACION (Monitor)  ** */
/* ************************************** */

SharedMinerData *create_miner_shm() {
  i32 fd = shm_open(MINER_SHM, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

  if (fd == ERR) {
    if (errno == EEXIST) {
      // Ya existe la memoria compartida, el monitor ya ha creado el sistema
      die_msg("El monitor ya ha creado la memoria compartida");
    } else
      die("shm_open");
  }

  if (ftruncate(fd, sizeof(SharedMinerData)) == ERR) {
    perror("ftruncate");
    close(fd);
    shm_unlink(MINER_SHM);
    exit(EXIT_FAILURE);
  }

  SharedMinerData *shared = mmap(NULL, sizeof(SharedMinerData),
                                 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  if (shared == MAP_FAILED) {
    perror("mmap");
    shm_unlink(MINER_SHM);
    exit(EXIT_FAILURE);
  }

  printf("Creates Shared Memory. My PID: %d\n", getpid());

  /* Inicializar datos */
  // Esto es necesario porque nuestro criterio de minero inactivo es pid == 0
  memset(shared, 0, sizeof(SharedMinerData));

  if (sem_init(&shared->mutex, 1, 1) == ERR ||
      sem_init(&shared->win, 1, 1) == ERR) {
    perror("sem_init shared");
    munmap(shared, sizeof(SharedMinerData));
    shm_unlink(MINER_SHM);
    exit(EXIT_FAILURE);
  }

  return shared;
}

SharedMonitorData *create_monitor_shm() {
  i32 fd = shm_open(MONITOR_SHM, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

  if (fd == ERR) {
    if (errno == EEXIST) {
      perror("El monitor ya ha creado la memoria compartida del buffer");
    } else {
      perror("shm_open");
    }
    return NULL;
  }

  if (ftruncate(fd, sizeof(SharedMonitorData)) == ERR) {
    perror("ftruncate");
    close(fd);
    shm_unlink(MONITOR_SHM);
    return NULL;
  }

  SharedMonitorData *shared = mmap(NULL, sizeof(SharedMonitorData),
                                   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  if (shared == MAP_FAILED) {
    perror("mmap");
    shm_unlink(MONITOR_SHM);
    return NULL;
  }

  memset(shared, 0, sizeof(SharedMonitorData));

  if (sem_init(&shared->mutex, 1, 1) == ERR) {
    perror("sem_init mutex");
    munmap(shared, sizeof(SharedMonitorData));
    shm_unlink(MONITOR_SHM);
    return NULL;
  }

  if (sem_init(&shared->empty, 1, MAX_MESSAGE_MONITOR) == ERR) {
    perror("sem_init empty");
    sem_destroy(&shared->mutex);
    munmap(shared, sizeof(SharedMonitorData));
    shm_unlink(MONITOR_SHM);
    return NULL;
  }

  if (sem_init(&shared->fill, 1, 0) == ERR) {
    perror("sem_init fill");
    sem_destroy(&shared->empty);
    sem_destroy(&shared->mutex);
    munmap(shared, sizeof(SharedMonitorData));
    shm_unlink(MONITOR_SHM);
    return NULL;
  }

  printf("Creates Monitor Shared Memory. My PID: %d\n", getpid());

  return shared;
}

/* ************************************** */
/* == FUNCIONES DE CONEXION (Mineros)  == */
/* ************************************** */

SharedMinerData *try_open_miner_shm() {
  i32 fd = shm_open(MINER_SHM, O_RDWR, 0);
  bool registered = false;

  if (fd == ERR) {
    if (errno == ENOENT) {
      // No existe la memoria compartida, el monitor no ha creado el sistema
      perror("Monitor no ha creado memoria compartida");
    } else
      perror("shm_open");

    return NULL;
  }

  SharedMinerData *shared = mmap(NULL, sizeof(SharedMinerData),
                                 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  if (shared == MAP_FAILED) {
    perror("mmap");
    return NULL;
  }

  printf("Got miner data. My PID: %d\n", getpid());

  /* REGION CRITICA */
  sem_wait(&shared->mutex);

  for (int i = 0; i < MAX_MINERS; i++) {
    if (shared->miners[i].miner_pid == 0) { // Se ha hallado hueco en la red
      shared->miners[i].miner_pid = getpid();
      shared->miners[i].coins = 0;
      shared->miners[i].current_vote = '\0';
      shared->active_miners++;
      registered = true;
      break;
    }
  }

  sem_post(&shared->mutex);

  if (registered == false) {
    munmap(shared, sizeof(SharedMinerData));
    fprintf(stderr, "Red llena, no se pudo añadir el minero\n");
    return NULL;
  }

  return shared;
}

SharedMonitorData *try_open_monitor_shm() {
  i32 fd = shm_open(MONITOR_SHM, O_RDWR, 0);

  if (fd == ERR) {
    if (errno == ENOENT) {
      perror("Monitor no ha creado memoria compartida del buffer");
    } else {
      perror("shm_open");
    }
    return NULL;
  }

  SharedMonitorData *shared = mmap(NULL, sizeof(SharedMonitorData),
                                   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  if (shared == MAP_FAILED) {
    perror("mmap");
    return NULL;
  }

  printf("Got monitor data. My PID: %d\n", getpid());

  return shared;
}

/* === FUNCIONES DE COLA DE MENSAJES === */

mqd_t create_miner_queue() {
  struct mq_attr attr;

  attr.mq_flags = 0;
  attr.mq_maxmsg = MAX_MESSAGE_MINER;
  attr.mq_msgsize = sizeof(MinerDataBlock);

  mq_unlink(MINER_MQ); /* fuerza inicializacion limpia */

  mqd_t mq =
      mq_open(MINER_MQ, O_CREAT | O_EXCL | O_RDONLY, S_IRUSR | S_IWUSR, &attr);
  if (mq == (mqd_t)ERR) {
    if (errno == EEXIST) {
      perror("Monitor ya ha creado la cola de mensajes");
    } else {
      perror("mq_open monitor");
    }
    return (mqd_t)ERR;
  }

  return mq;
}

mqd_t try_open_miner_queue() {
  mqd_t mq = mq_open(MINER_MQ, O_WRONLY);
  if (mq == (mqd_t)ERR) {
    if (errno == ENOENT) {
      perror("Monitor no ha creado la cola de mensajes");
    } else
      perror("mq_open monitor");
    return (mqd_t)ERR;
  }

  return mq;
}

/* ************************************** */
/* ======== FUNCIONES DE CLEANUP ======== */
/* ************************************** */

void detach_miner_shm(SharedMinerData *shared) {
  if (shared != NULL && shared != MAP_FAILED) {

    /* ZONA CRITICA */
    sem_wait(&shared->mutex);

    for (int i = 0; i < MAX_MINERS; i++) {
      if (shared->miners[i].miner_pid == getpid()) {
        shared->miners[i].miner_pid = 0; // Liberamos el hueco
        shared->active_miners--;
        break;
      }
    }

    sem_post(&shared->mutex);

    munmap(shared, sizeof(SharedMinerData));
  }
}

void destroy_miner_shm(SharedMinerData *shared) {
  if (shared != NULL && shared != MAP_FAILED) {
    sem_destroy(&shared->mutex);
    sem_destroy(&shared->win);
    munmap(shared, sizeof(SharedMinerData));
  }
  shm_unlink(MINER_SHM);
}

void destroy_monitor_shm(SharedMonitorData *shared) {
  if (shared != NULL && shared != MAP_FAILED) {
    sem_destroy(&shared->mutex);
    sem_destroy(&shared->empty);
    sem_destroy(&shared->fill);
    munmap(shared, sizeof(SharedMonitorData));
  }
  shm_unlink(MONITOR_SHM);
}

void destroy_miner_queue(mqd_t mq) {
  if (mq != (mqd_t)ERR) {
    mq_close(mq);
  }
  mq_unlink(MINER_MQ);
}
