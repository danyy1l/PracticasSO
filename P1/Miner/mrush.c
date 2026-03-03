/**
 * @file mrush.c
 * @author Danyyil Shykerynets
 * @brief Punto de entrada del programa
 * * Contiene el main a ejecutar del programa
 * @version 1.0
 * @date 2026-02-09
 */

#include "logger.h"
#include "miner.h"
#include "types.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  /* CONTROL DE ARGUMENTOS */
  if (argc != 4) {
    fprintf(stderr, "Miner exited unexpectedly!\n");
    die("Usage: ./miner <TARGET_INI> <ROUNDS> <N_THREADS>");
  }

  if (atoi(argv[1]) < 0 || atoi(argv[2]) < 0 || atoi(argv[3]) < 0) {
    die("Target, rounds and number of threads MUST be positive\n");
  }

  /* INICIALIZACION DE VARIABLES */
  i32 miner_pipe[2] = {0};
  i32 logger_pipe[2] = {0};
  Miner_data data_miner = {0};
  i32 status = EXIT_FAILURE;

  data_miner.target = atoi(argv[1]);
  data_miner.rounds = atoi(argv[2]);
  data_miner.n_threads = atoi(argv[3]);

  /* APERTURA DE PIPES */
  open_pipes(miner_pipe, logger_pipe);

  /* FORK */
  pid_t childpid = fork();
  if (childpid < 0) {
    int x = errno;
    printf("Miner exited unexpectedly!\n");
    die(strerror(x));
  } else if (childpid == 0) {
    /* Control de extremos de las tuberias */
    close(logger_pipe[READ]);
    close(miner_pipe[WRITE]);

    status = logger(miner_pipe, logger_pipe);
    printf("Logger exited with status %d\n", status);

    close(logger_pipe[WRITE]);
    close(miner_pipe[READ]);
    exit(status);
  } else {
    /* Control de extremos de las tuberias */
    close(miner_pipe[READ]);
    close(logger_pipe[WRITE]);

    minero(&data_miner, miner_pipe, logger_pipe);

    close(miner_pipe[WRITE]);
    close(logger_pipe[READ]);

    waitpid(childpid, NULL, 0);
  }

  printf("Miner exited with status 0\n");

  return EXIT_SUCCESS;
}
