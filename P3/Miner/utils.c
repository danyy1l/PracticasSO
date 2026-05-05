/**
 * @file utils.c
 * @author Danyyil Shykerynets
 * @brief Implementacion de utilidades
 * * Implementa las funciones utiles en el proyecto
 * @version 1.0
 * @date 2026-04-02
 *
 * @copyright (c) 2026 Author. All Rights Reserved.
 */

#include "utils.h"
#include "types.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void open_pipes(i32 *miner_pipe, i32 *logger_pipe) {
  assert(miner_pipe != NULL);
  assert(logger_pipe != NULL);

  /* APERTURA TUBERIA MINER ---> LOGGER */
  if (pipe(miner_pipe) == ERR)
    die("Error al abrir tuberia del minero");

  /* APERTURA TUBERIA LOGGER ---> MINER */
  if (pipe(logger_pipe) == ERR) {
    close(miner_pipe[READ]);
    close(miner_pipe[WRITE]);
    die("Error al abrir la tuberia del registrador");
  }
}

void close_pipes(i32 *miner_pipe, i32 *logger_pipe) {
  close(logger_pipe[READ]);
  close(logger_pipe[WRITE]);

  /* Ponemos a -1 por si se vuelve a llamar la funcion */
  logger_pipe[READ] = -1;
  logger_pipe[WRITE] = -1;

  close(miner_pipe[READ]);
  close(miner_pipe[WRITE]);

  miner_pipe[READ] = -1;
  miner_pipe[WRITE] = -1;
}

i32 write_pid_unlocked(const char *filename) {
  FILE *fp = NULL;
  if ((fp = fopen(filename, "a")) == NULL)
    return ERR;

  fprintf(fp, "%d\n", getpid());
  fclose(fp);

  return OK;
}

u64 str_to_u64(char *input) {
  char *endptr;

  /* No usamos atoi porque los numeros usados son my grandes, es posible que
   * haya overflow */
  u64 out = strtoull(input, &endptr, 10);

  /* En el manual dice que en caso de desbordamiento strtoul anota ERANGE en
   * errno. Como nuestros datos se guardan en u64, si tambien fuera negativo lo
   * convertiria a positivo y desbordaria */
  if (errno == ERANGE)
    die("Desbordamiento! El numero introducido es demasiado grande o negativo");

  if (strchr(input, '-'))
    die_msg("No se aceptan parámetros negativos");

  /* strtoul coloca endptr en el primer digito que no sea un numero luego si
   * coincide en el principio es que no habia ningun numero valido */
  if (endptr == input)
    die("Parámetro inválido, debe ser solo un numero");

  if (*endptr != '\0')
    die("Parámetro inválido, debe ser solo un número");

  return out;
}
