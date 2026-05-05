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
#include "shared.h"
#include "types.h"
#include "utils.h"
#include <fcntl.h>
#include <mqueue.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * Parsea un numero en string a unsigned int de 64 bit de forma segura
 *
 * @param input String a parsear
 * @return Unsigned int de 64 bit o ERR en su caso
 */
u64 str_to_u64(char *input);

int main(int argc, char *argv[]) {
  /* CONTROL DE ARGUMENTOS */
  if (argc != 3) {
    fprintf(stderr, "Miner exited unexpectedly!\n");
    die_msg("Usage: ./miner <N_SECS> <N_THREADS>");
  }

  /* INICIALIZACION DE VARIABLES */
  i32 miner_pipe[2] = {-1, -1};
  i32 logger_pipe[2] = {-1, -1};
  Miner_data data_miner = {0};
  i32 status = EXIT_FAILURE;

  data_miner.time = str_to_u64(argv[1]);
  data_miner.n_threads = str_to_u64(argv[2]);

  /*SHARED MEMORY*/
  SharedMinerData *shared_data = try_open_miner_shm();
  if (shared_data == NULL) {
    die_msg("No se pudo abrir la memoria compartida\n");
  }

  mqd_t miner_mq = try_open_miner_queue();
  if (miner_mq == (mqd_t)ERR) {
    detach_miner_shm(shared_data);
    die_msg("No se pudo enlazar la cola de mensajes\n");
  }

  /* APERTURA DE PIPES */
  open_pipes(miner_pipe, logger_pipe);

  /* FORK */
  pid_t childpid = fork();
  if (childpid < 0) {
    perror("Miner exited unexpectedly!");
    close_pipes(miner_pipe, logger_pipe);
    detach_miner_shm(shared_data);
    exit(EXIT_FAILURE);

  } else if (childpid == 0) {
    /* esto lo hereda del minero */
    detach_miner_shm(shared_data);
    mq_close(miner_mq);

    /* Control de extremos de las tuberias */
    close(logger_pipe[READ]);
    close(miner_pipe[WRITE]);

    status = logger(miner_pipe, logger_pipe);

    close(logger_pipe[WRITE]);
    close(miner_pipe[READ]);
    exit(status);
  } else {
    /* Control de extremos de las tuberias */
    close(miner_pipe[READ]);
    close(logger_pipe[WRITE]);

    minero(&data_miner, miner_pipe, logger_pipe, shared_data, miner_mq);

    close(miner_pipe[WRITE]);
    close(logger_pipe[READ]);

    waitpid(childpid, NULL, 0);
  }

  detach_miner_shm(shared_data);
  mq_close(miner_mq);

  return EXIT_SUCCESS;
}
