/**
 * @file mrush.c
 * @author Danyyil Shykerynets
 * @brief Punto de entrada del programa
 * * Contiene el main a ejecutar del programa
 * @version 1.0
 * @date 2026-02-09
 */

#include "files.h"
#include "logger.h"
#include "miner.h"
#include "types.h"
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define TARGET_INIT 0 /**< Valor de inicializacion del target */

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

  /* INICIALIZACION DE SEMAFOROS */
  Miner_Mutexes sems;
  initialize_mutexes(&sems);

  data_miner.time = str_to_u64(argv[1]);
  data_miner.n_threads = str_to_u64(argv[2]);

  /* INICIALIZACION DE TARGET */
  sem_wait(sems.tgt);
  if (write_target_unlocked(TARGET_FILE, TARGET_INIT) == ERR) {
    sem_post(sems.tgt);
    die("write_target");
  }
  sem_post(sems.tgt);

  /* APERTURA DE PIPES */
  open_pipes(miner_pipe, logger_pipe);

  /* FORK */
  pid_t childpid = fork();
  if (childpid < 0) {
    int err = errno;
    close_pipes(miner_pipe, logger_pipe);
    sem_close(sems.pid);
    fprintf(stderr, "Miner exited unexpectedly!: %s", strerror(err));
    exit(EXIT_FAILURE);

  } else if (childpid == 0) {
    /* Control de extremos de las tuberias */
    close(logger_pipe[READ]);
    close(miner_pipe[WRITE]);

    status = logger(miner_pipe, logger_pipe);
    // printf("Logger exited with status %d\n", status);

    close(logger_pipe[WRITE]);
    close(miner_pipe[READ]);
    exit(status);
  } else {
    /* Control de extremos de las tuberias */
    close(miner_pipe[READ]);
    close(logger_pipe[WRITE]);

    minero(&data_miner, miner_pipe, logger_pipe, &sems);

    close(miner_pipe[WRITE]);
    close(logger_pipe[READ]);

    waitpid(childpid, NULL, 0);
  }

  // printf("Miner exited with status %d\n", EXIT_SUCCESS);

  return EXIT_SUCCESS;
}

u64 str_to_u64(char *input) {
  char *endptr;

  /* No usamos atoi porque los numeros usados son my grandes, es posible que
   * haya overflow */
  u64 out = strtoul(input, &endptr, 10);

  /* En el manual dice que en caso de desbordamiento strtoul anota ERANGE en
   * errno. Como nuestros datos se guardan en u64, si tambien fuera negativo lo
   * convertiria a positivo y desbordaria */
  if (errno == ERANGE)
    die("Desbordamiento! El numero introducido es demasiado grande o negativo");

  /* strtoul coloca endptr en el primer digito que no sea un numero luego si
   * coincide en el principio es que no habia ningun numero valido */
  if (endptr == input)
    die("Parámetro inválido, debe ser solo un numero");

  if (*endptr != '\0')
    die("Parámetro inválido, debe ser solo un número");

  return out;
}
