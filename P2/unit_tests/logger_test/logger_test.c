#include "logger_test.h"
#include "file_utils.h"
#include "logger.h"
#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  printf("Running Logger tests...\n");
  test1_logger_ipc();
  PRINT_PASSED_PERCENTAGE;
  return EXIT_SUCCESS;
}

void test1_logger_ipc() {
  int miner_pipe[2], logger_pipe[2];
  open_pipes(miner_pipe, logger_pipe);

  pid_t pid = fork();
  if (pid == 0) {
    close(logger_pipe[READ]);
    close(miner_pipe[WRITE]);
    logger(miner_pipe, logger_pipe);
    close(logger_pipe[WRITE]);
    close(miner_pipe[READ]);
    exit(0);
  } else {
    close(miner_pipe[READ]);
    close(logger_pipe[WRITE]);

    Logger_args args = {0};
    args.id = 1;
    args.target = 1234;
    args.solution = 5678;
    args.pos_votes = 3;
    args.votes = 4;
    args.wallets = 1;
    args.winner = getpid();
    args.validated = 1;

    write(miner_pipe[WRITE], &args, sizeof(Logger_args));
    int status;
    read(logger_pipe[READ], &status, sizeof(int));

    /* Enviamos fin (ERR) */
    args.target = (u64)ERR;
    write(miner_pipe[WRITE], &args, sizeof(Logger_args));

    wait(NULL);
    close(miner_pipe[WRITE]);
    close(logger_pipe[READ]);

    /* Comprobamos que el archivo log existe y tiene datos */
    char filename[64];
    snprintf(filename, sizeof(filename), "%d.log", getpid());
    FILE *f = fopen(filename, "r");
    PRINT_TEST_RESULT(f != NULL);
    if (f) {
      fseek(f, 0, SEEK_END);
      PRINT_TEST_RESULT(ftell(f) > 0);
      fclose(f);
      unlink(filename);
    }
  }
}
