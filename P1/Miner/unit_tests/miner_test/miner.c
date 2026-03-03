/**
 * @file miner.c
 * @author Fernando Blanco & Danyyil Shykerynets
 * @brief Implementación de minero.
 * * Contiene la implementación de las funciones de cálculo del POW, así como la
 * lógica del miner implementación de las funciones de cálculo del POW, así como
 * la lógica del minero y el IPC entre minero y registrador
 * @version 1.0
 * @date 2026-02-12
 *
 * @copyright (c) 2026 Author. All Rights Reserved.
 */

#include "miner.h"
#include "logger.h"
#include "pow.h"
#include "types.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/************************** FUNCIONES PRIVADAS ****************************/

/**
 * @brief Manda un mensaje a logger
 * Función auxiliar para minero
 *
 * @param miner_pipe Tuberia de escritura del minero al registrador
 * @param args Estructura de argumentos del logger
 */
void comunicar_logger(i32 *miner_pipe, Logger_args *args) {
  /* No es necesario comprobar argumentos dado que vienen de funcion minero */
  write(miner_pipe[WRITE], args, sizeof(Logger_args));
}

/**************************************************************************/
/*----------------------- IMPLEMENTACION FUNCIONES -----------------------*/
/**************************************************************************/

void open_pipes(i32 *miner_pipe, i32 *logger_pipe) {
  assert(miner_pipe != NULL);
  assert(logger_pipe != NULL);

  /* APERTURA TUBERIA MINER ---> LOGGER */
  if (pipe(miner_pipe) == ERR)
    die(strerror(errno));

  /* APERTURA TUBERIA LOGGER ---> MINER */
  if (pipe(logger_pipe) == ERR)
    die(strerror(errno));
}

void *pow_seek(void *arg) {
  assert(arg != NULL);

  Arg_hilos *args = (Arg_hilos *)arg;
  if (*(args->found_value) == FOUND)
    return NULL;

  u64 i = 0;
  u64 *pow_result = (u64 *)malloc(sizeof(u64));
  if (pow_result == NULL)
    die(strerror(errno));

  for (i = args->min; i <= args->max; i++) {
    if (*(args->found_value) == FOUND)
      break;

    if (pow_hash(i) == args->target) {
      *(args->found_value) = FOUND;
      *pow_result = i;
      printf("Solution accepted: %08lu --> %08lu\n", args->target, i);
      return pow_result;
    }
  }

  free(pow_result);
  return NULL;
}

u64 calcular_solucion(Miner_data *args) {
  /* VERIFICACION PARAMETROS DE ENTRADA */
  assert(args->n_threads > 0);
  assert(args->rounds > 0);

  /* INICIALIZO ARGUMENTOS DE POW_SEEK Y ARRAY DE HILOS  */
  pthread_t *hilos = (pthread_t *)malloc(args->n_threads * sizeof(pthread_t));
  if (hilos == NULL)
    die(strerror(errno));

  Arg_hilos *thread_args =
      (Arg_hilos *)malloc(args->n_threads * sizeof(Arg_hilos));
  if (thread_args == NULL)
    die(strerror(errno));

  volatile int found_flag = 0;
  u64 rango_busqueda = POW_LIMIT / args->n_threads;

  /* CREACION DE HILOS */
  for (u64 i = 0; i < args->n_threads; i++) {
    thread_args[i].target = args->target;
    thread_args[i].min = i * rango_busqueda;
    /* Con el ternario aseguro que en el ultimo rango de todos llegue hasta el
     * limite, podria quedarse corto */
    thread_args[i].max = (i == args->n_threads - 1)
                             ? (POW_LIMIT - 1)
                             : (i + 1) * rango_busqueda - 1;
    thread_args[i].found_value = &found_flag;
    /* Arg debe ser void* por eso hay que castear el puntero al hilo */
    if (pthread_create(&hilos[i], NULL, pow_seek, (void *)&thread_args[i]) !=
        OK) {
      int foo = errno;
      free(hilos);
      die(strerror(foo));
    }
  }

  u64 sol = 0;
  _Bool found = 0;

  for (u64 i = 0; i < args->n_threads; i++) {
    /*thread_return coge un void ** */
    void *valor_retorno;
    pthread_join(hilos[i], &valor_retorno);

    if (valor_retorno != NULL) {
      u64 *out = (u64 *)valor_retorno;
      sol = *out;
      found = 1;

      free(out);
    }
  }

  /* LIBERACION DE MEMORIA */
  free(hilos);
  free(thread_args);
  // Casteamos a unsigned long int para prevenir warning
  return (found == 1) ? sol : (u64)ERR;
}

void minero(Miner_data *args, i32 *miner_pipe, i32 *logger_pipe) {
  assert(args != NULL);
  assert(miner_pipe != NULL);
  assert(logger_pipe != NULL);

  u64 ronda_actual = 1;
  Logger_args logger_args = {0};
  logger_args.winner = getpid();
  logger_args.validated = 1;

  do {
    u64 sol = calcular_solucion(args);
    i32 status;

    logger_args.target = args->target;
    logger_args.id = ronda_actual;
    logger_args.solution = sol;
    logger_args.validated = 1;
    logger_args.votes = ronda_actual;
    logger_args.wallets = logger_args.winner;

    comunicar_logger(miner_pipe, &logger_args);

    if (read(logger_pipe[READ], &status, sizeof(i32)) <= 0)
      break;

    args->target = sol;
    ronda_actual++;
  } while (args->target != (u64)ERR && ronda_actual <= args->rounds);

  /* Mando señal de finalizacion */
  logger_args.target = (u64)ERR;
  comunicar_logger(miner_pipe, &logger_args);
}
