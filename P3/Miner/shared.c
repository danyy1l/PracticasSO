#include "types.h"
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define MINER_SHM "/miner_data"
#define MONITOR_SHM "/monitor_data"

typedef struct {
  sem_t mutex;
  pid_t miner_pids[MAX_MINERS];
  u64 miner_target;
  char votes[MAX_MINERS];
  u64 active_miners;
} SharedMinerData;

SharedMinerData *try_open_miner() {
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

  printf("Got miner data. My PID: %d\n", getpid());

  // TODO: Apuntar PID, unmap
  //
  return shared;
}

// TODO: Exit y unlink, eliminar pid al salir, etc etc
