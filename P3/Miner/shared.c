
#include "shared.h"
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mqueue.h>


/* Shared Memory Functions */

SharedMinerData *create_miner_shm() {
  i32 fd = shm_open(MINER_SHM, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

  if (fd == ERR) {
    if (errno == EEXIST) {
      // Ya existe la memoria compartida, el monitor ya ha creado el sistema
      die_msg("El monitor ya ha creado la memoria compartida");
    } else
      die("shm_open");

  } else {
    if (ftruncate(fd, sizeof(SharedMinerData)) == ERR) {
      perror("ftruncate");
      close(fd);
      exit(EXIT_FAILURE);
    }
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
  memset(shared, 0, sizeof(SharedMinerData));

  if (sem_init(&shared->mutex, 1, 1) == ERR) {
    perror("sem_init mutex");
    munmap(shared, sizeof(SharedMinerData));
    shm_unlink(MINER_SHM);
    exit(EXIT_FAILURE);
  }

  shared->miner_count = 0;
  shared->miner_target = 0;
  shared->active_miners = 0;

  return shared;
}

SharedMinerData *try_open_miner_shm() {
  i32 fd = shm_open(MINER_SHM, O_RDWR, 0);

  if (fd == ERR) {
    if (errno == ENOENT) {
      // No existe la memoria compartida, el monitor no ha creado el sistema
      die_msg("Monitor no ha creado memoria compartida");
    } else
      die("shm_open");
  }

  SharedMinerData *shared = mmap(NULL, sizeof(SharedMinerData),
                                 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  if (shared == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  printf("Got miner data. My PID: %d\n", getpid());

  sem_wait(&shared->mutex);
  shared->miner_count++;
  shared->active_miners++;
  shared->miner_pids[shared->miner_count - 1] = getpid();
  shared->miner_wallets[shared->miner_count - 1].miner_pid = getpid();
  shared->miner_wallets[shared->miner_count - 1].coins = 0;
  sem_post(&shared->mutex);

  return shared;
}

SharedMonitorData *create_monitor_shm() {
  i32 fd =
      shm_open(MONITOR_SHM, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

  if (fd == ERR) {
    if (errno == EEXIST) {
      die_msg("El monitor ya ha creado la memoria compartida del buffer");
    } else {
      die("shm_open");
    }
  }

  if (ftruncate(fd, sizeof(SharedMonitorData)) == ERR) {
    perror("ftruncate");
    close(fd);
    shm_unlink(MONITOR_SHM);
    exit(EXIT_FAILURE);
  }

  SharedMonitorData *shared =
      mmap(NULL, sizeof(SharedMonitorData), PROT_READ | PROT_WRITE, MAP_SHARED,
           fd, 0);
  close(fd);

  if (shared == MAP_FAILED) {
    perror("mmap");
    shm_unlink(MONITOR_SHM);
    exit(EXIT_FAILURE);
  }

  memset(shared, 0, sizeof(SharedMonitorData));

  if (sem_init(&shared->mutex, 1, 1) == ERR) {
    perror("sem_init mutex");
    munmap(shared, sizeof(SharedMonitorData));
    shm_unlink(MONITOR_SHM);
    exit(EXIT_FAILURE);
  }

  if (sem_init(&shared->empty, 1, MAX_MESSAGE_MONITOR) == ERR) {
    perror("sem_init empty");
    sem_destroy(&shared->mutex);
    munmap(shared, sizeof(SharedMonitorData));
    shm_unlink(MONITOR_SHM);
    exit(EXIT_FAILURE);
  }

  if (sem_init(&shared->fill, 1, 0) == ERR) {
    perror("sem_init fill");
    sem_destroy(&shared->empty);
    sem_destroy(&shared->mutex);
    munmap(shared, sizeof(SharedMonitorData));
    shm_unlink(MONITOR_SHM);
    exit(EXIT_FAILURE);
  }

  shared->write_idx = 0;
  shared->read_idx = 0;

  printf("Creates Monitor Shared Memory. My PID: %d\n", getpid());

  return shared;
}

SharedMonitorData *try_open_monitor_shm() {
  i32 fd = shm_open(MONITOR_SHM, O_RDWR, 0);

  if (fd == ERR) {
    if (errno == ENOENT) {
      die_msg("Monitor no ha creado memoria compartida del buffer");
    } else {
      die("shm_open");
    }
  }

  SharedMonitorData *shared =
      mmap(NULL, sizeof(SharedMonitorData), PROT_READ | PROT_WRITE, MAP_SHARED,
           fd, 0);
  close(fd);

  if (shared == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  printf("Got monitor data. My PID: %d\n", getpid());

  return shared;
}



/* Message Queue Functions */

mqd_t create_miner_queue() {
  struct mq_attr attr;

  attr.mq_flags = 0;
  attr.mq_maxmsg = MAX_MESSAGE_MINER;
  attr.mq_msgsize = sizeof(MinerDataBlock);

  mq_unlink(MINER_MQ); /* fuerza inicializacion limpia */

  mqd_t mq = mq_open(MINER_MQ, O_CREAT | O_EXCL | O_RDONLY,
                     S_IRUSR | S_IWUSR, &attr);
  if (mq == (mqd_t)-1) {
    if (errno == EEXIST) {
      die_msg("Monitor ya ha creado la cola de mensajes");
    } else {
      die("mq_open monitor");
    }
  }

  return mq;
}

mqd_t try_open_miner_queue() {
  mqd_t mq = mq_open(MINER_MQ, O_RDONLY);
  if (mq == (mqd_t)-1) {
    if (errno == ENOENT) {
      die_msg("Monitor no ha creado la cola de mensajes");
    } else
      die("mq_open monitor");
  }

  return mq;
}


// TODO: Exit y unlink, eliminar pid al salir, etc etc
