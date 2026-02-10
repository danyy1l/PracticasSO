#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void miner() { printf("I am Miner!\n"); }

int main(int argc, char *argv[]) {
  /* CONTROL DE ARGUMENTOS */
  if (argc != 4) {
    perror("Usage: ./miner <TARGET_INI> <ROUNDS> <N_THREADS>");
    printf("Miner exited unexpectedly!\n");
    exit(EXIT_FAILURE);
  }

  /* INICIALIZACION DE VARIABLES */
  u32 rounds = atoi(argv[2]);
  u32 n_threads = atoi(argv[3]);
  u32 status = 0;
  pid_t rc = 0;
  char *args[2] = {"./logger", "NULL"};

  /* FORK */
  rc = fork();
  if (rc < 0) {
    perror("Error! Couldn't initialize new process\n");
    printf("Miner exited unexpectedly!\n");
    exit(EXIT_FAILURE);
  } else if (rc == 0) {
    execv("./logger", args);
    exit(EXIT_SUCCESS);
  } else {
    status = wait(NULL);
    if (status) {
      printf("Logger exited with status 0\n");
    } else {
      printf("Logger exited unexpectedly!\n");
    }
    miner();
  }

  printf("Miner exited with status 0\n");

  return EXIT_SUCCESS;
}
