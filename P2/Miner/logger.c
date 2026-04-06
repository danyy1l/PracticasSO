/**
 * @file logger.c
 * @author Fernando Blanco & Danyyil Shykerynets
 * @brief Implementación de registrador.
 * * Contiene la implementación y lógica de registro en fichero de disco.
 * @version 1.0
 * @date 2026-02-25
 *
 * @copyright (c) 2026 Author. All Rights Reserved.
 */

#include "logger.h"
#include "files.h"
#include "types.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

i32 logger(i32 *miner_pipe, i32 *logger_pipe) {
  /* VERIFICACION PARAMETROS DE ENTRADA */
  assert(miner_pipe != NULL);
  assert(logger_pipe != NULL);

  char filename[64];
  Logger_args arg;
  i32 status = OK;

  /* APERTURA DE DESCRIPTOR DE ARCHIVO  */
  snprintf(filename, sizeof(filename), "%d.log", getppid());
  /* Abrimos fd con permisos de RW propietario y solo Read para el resto */
  int fd_logger = open(filename, O_CREAT | O_WRONLY | O_APPEND, 0644);
  if (fd_logger == ERR)
    return EXIT_FAILURE;

  /* Escritura en archivo mientras Minero no mande EOF */
  while (read(miner_pipe[READ], &arg, sizeof(Logger_args)) > 0) {
    if (arg.target == (u64)ERR)
      break;

    dprintf(fd_logger, "%-12s%lu\n", "Id:", arg.id);
    dprintf(fd_logger, "%-12s%lu\n", "Winner:", (u64)arg.winner);
    dprintf(fd_logger, "%-12s%08lu\n", "Target:", arg.target);

    if (arg.validated) {
      dprintf(fd_logger, "%-12s%08lu %s", "Solution:", arg.solution,
              "(validated)\n");
    } else {
      dprintf(fd_logger, "%-12s%08lu %s", "Solution:", arg.solution,
              "(rejected)\n");
    }

    dprintf(fd_logger, "%-12s%lu/%lu\n", "Votes:", arg.pos_votes, arg.votes);
    dprintf(fd_logger, "%-12s%lu:%lu\n\n", "Wallets:", (u64)arg.winner,
            arg.wallets);

    if (write(logger_pipe[WRITE], &status, sizeof(i32)) == ERR) {
      status = EXIT_FAILURE;
      break;
    }
  }

  close(fd_logger);
  return status;
}
