
#include "shared.h"
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mqueue.h>


/* Shared Memory Functions */

SharedMinerData *create_miner_shm() {
  i32 fd = shm_open(MINER_SHM, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  bool created = false;

  if (fd == ERR) {
    if (errno == EEXIST) {
      if ((fd = shm_open(MINER_SHM, O_RDWR, 0)) == ERR)
        die("shm_open");

    } else
      die("shm_open");

  } else {
    created = true;
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
  sem_init(&shared->mutex, 1, 1);
  shared->miner_count = 0;
  shared->miner_target = 0;
  shared->active_miners = 0;

  return shared;
}

SharedMinerData *try_open_miner() {
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

  return shared;
}

/* Message Queue Functions */

mqd_t create_miner_queue() {
  struct mq_attr attr;

  attr.mq_flags = 0;
  attr.mq_maxmsg = MAX_MESSAGE_MINER;
  attr.mq_msgsize = sizeof(MinerBlock);

  mq_unlink(MINER_MQ); /* fuerza inicializacion limpia */

  mqd_t mq = mq_open(MINER_MQ, O_CREAT | O_RDONLY,
                     S_IRUSR | S_IWUSR, &attr);
  if (mq == (mqd_t)-1)
    die("mq_open monitor");

  return mq;
}

// TODO: Exit y unlink, eliminar pid al salir, etc etc
