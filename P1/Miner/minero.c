#include "minero.h"
#include "pow.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/**************************************************************************/
/*----------------------- IMPLEMENTACION FUNCIONES -----------------------*/
/**************************************************************************/

void open_pipes(i32 *miner_pipe, i32 *logger_pipe) {
  /* APERTURA TUBERIA MINER ---> LOGGER */
  if (pipe(miner_pipe) == ERR)
    die(strerror(errno));

  /* APERTURA TUBERIA LOGGER ---> MINER */
  if (pipe(logger_pipe) == ERR)
    die(strerror(errno));
}

void close_pipes(i32 *miner_pipe, i32 *logger_pipe) {
  close(miner_pipe[READ]);
  close(miner_pipe[WRITE]);
  close(logger_pipe[READ]);
  close(logger_pipe[WRITE]);
}

void *pow_seek(void *arg) {
  Arg_hilos *args = (Arg_hilos *)arg;
  u64 *pow_result = (u64 *)malloc(sizeof(u64));
  if (pow_result == NULL)
    die(strerror(errno));

  for (u64 i = args->min; i <= args->max; i++) {
    if (*(args->found_value) == FOUND)
      break;

    if (pow_hash(i) == args->target) {
      *(args->found_value) = FOUND;
      *pow_result = i;
      printf("Res is: %ld\n", i);
      return pow_result;
    }
  }

  free(pow_result);
  return NULL;
}

u64 create_threads(i64 target, u64 rounds, u64 n_threads) {
  /* VERIFICACION PARAMETROS DE ENTRADA */
  assert(n_threads > 0);
  assert(rounds > 0);

  /* INICIALIZO ARGUMENTOS DE POW_SEEK Y ARRAY DE HILOS  */
  pthread_t *hilos = (pthread_t *)malloc(n_threads * sizeof(pthread_t));
  if (hilos == NULL)
    die(strerror(errno));

  Arg_hilos *thread_args = (Arg_hilos *)malloc(n_threads * sizeof(Arg_hilos));
  if (thread_args == NULL)
    die(strerror(errno));

  volatile int found_flag = 0;
  u64 rango_busqueda = POW_LIMIT / n_threads;

  /* CREACION DE HILOS */
  for (u64 i = 0; i < n_threads; i++) {
    thread_args[i].target = target;
    thread_args[i].min = i * rango_busqueda;
    /* Con el ternario aseguro que en el ultimo rango de todos llegue hasta el
     * limite, podria quedarse corto */
    thread_args[i].max =
        (i == n_threads - 1) ? (POW_LIMIT - 1) : (i + 1) * rango_busqueda - 1;
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

  for (u64 i = 0; i < n_threads; i++) {
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
  return (found == 1) ? sol : (u64)ERR;
}

void minero(i64 target, u64 rounds, u64 n_threads) {
  u64 new_target = target;
  u64 i = 0;

  printf("I am Miner!\n");

  do {
    new_target = create_threads(new_target, rounds, n_threads);
    i++;
  } while (new_target != (u64)ERR && i < rounds);
}
