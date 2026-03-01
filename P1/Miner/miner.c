#include "logger.h"
#include "minero.h"
#include "types.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  /* CONTROL DE ARGUMENTOS */
  if (argc != 4) {
    fprintf(stderr, "Miner exited unexpectedly!\n");
    die("Usage: ./miner <TARGET_INI> <ROUNDS> <N_THREADS>");
  }

  /* INICIALIZACION DE VARIABLES */
  i32 miner_pipe[2] = {0};
  i32 logger_pipe[2] = {0};
  i64 target = atoi(argv[1]);
  u64 rounds = atoi(argv[2]);
  u64 n_threads = atoi(argv[3]);

  /* APERTURA DE PIPES */
  open_pipes(miner_pipe, logger_pipe);

  /* FORK */
  pid_t childpid = fork();
  if (childpid < 0) {
    int x = errno;
    printf("Miner exited unexpectedly!\n");
    die(strerror(x));
  } else if (childpid == 0) {
    logger();
    exit(EXIT_SUCCESS);
  } else {
    minero(target, rounds, n_threads);
    waitpid(childpid, NULL, 0);
  }

  printf("Miner exited with status 0\n");

  /* CERRAMOS PIPES */
  close_pipes(miner_pipe, logger_pipe);
  return EXIT_SUCCESS;
}
