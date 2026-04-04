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
#include "files.h"
#include "logger.h"
#include "pow.h"
#include "types.h"
#include <assert.h>
#include <bits/types/sigset_t.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TARGET_INIT 0 /**< Valor de inicializacion del target */

/**************************************************************************/
/*-------------------------- FUNCIONES PRIVADAS --------------------------*/
/**************************************************************************/

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

/**
 * @brief El minero abandona la red
 * Abandona la red, libera la memoria dedicada a los semaforos por el proceso, y
 * si es el ultimo, elimina todas las referencias a los semaforos para que no
 * queden activos en shm
 *
 * @param filename Fichero con PIDs de toda la red
 * @param mutex_pid MutEx que protege fichero con PIDs
 * @param mutex_tgt MutEx que protege fichero con target
 * @param mutex_vot MutEx que protege fichero con votos
 * @param mutex_winner MutEx que protege al ganador de ronda
 */
void exit_network(const char *filename, Miner_Mutexes *sems) {
  pid_t active_miners[256];

  sem_wait(sems->pid);

  i32 n_active =
      get_active_pids_unlocked(filename, active_miners, getpid(), true);

  if (n_active == 0) {
    printf("Miner %d exited. No miners left in system.\n", getpid());

    unlink(TARGET_FILE);
    unlink(VOTES_FILE);
    unlink(PID_FILE);

    sem_unlink(PID_MUTEX);
    sem_unlink(TARGET_MUTEX);
    sem_unlink(VOTES_MUTEX);
    sem_unlink(WINNER_MUTEX);
  } else {
    printf("Miner %d exited system.\n\n", getpid());
    printf("===== ACTIVE MINERS =====\n");

    for (i32 i = 0; i < n_active; i++)
      printf("- Process %7d\n", active_miners[i]);

    printf("\n");
  }

  sem_post(sems->pid);

  /* Liberamos la memoria que usaba el proceso para los semaforos */
  sem_close(sems->tgt);
  sem_close(sems->vot);
  sem_close(sems->win);
  sem_close(sems->pid);
}

/*******************************************/
/*------- IMPLEMENTACION SEÑALES -----------*/
/*******************************************/

/**
 * Flag de timeout del minero
 * Mientras sea 0, el minero trabaja, en cuanto se acabe el tiempo y se reciba
 * SIGALARM, se pone a 1 y el minero abandona la red
 */
volatile sig_atomic_t timeout = 0;

/**
 * Flag de comienzo de mineria
 * Mientras sea 0, los mineros esperan
 * Una vez a 1, comienza la ronda de mineria
 * Se reseteara a 0 tras el comienzo de la ronda
 */
volatile sig_atomic_t start_mining = 0;

/**
 * Flag de comienzo de votacion
 * Mientras este a 0, los mineros esperan y no votan
 * Cuando se ponga a 1, comienza la ronda de votacion
 * Se reseteara a 0 tras el comienzo de las votaciones
 */
volatile sig_atomic_t start_voting = 0;

/**
 * @brief Manejador de señales
 * Coloca el flag de timeout a 1, indicando que el minero ha terminado
 * Coloca el flag de start_mining a 1, comenzando ronda de mineria
 * Coloca el flag de start_voting a 1, comenzando ronda de votacion
 *
 * @param sig Numero de la señal a manejar (SIGALARM, SIGUSR1 o SIGUSR2)
 */
void handler(int sig) {
  switch (sig) {
  case SIGALRM:
    timeout = 1;
    break;
  case SIGUSR1:
    start_mining = 1;
    break;
  case SIGUSR2:
    start_voting = 1;
    break;
  }
}

/**
 * @brief Hace el setup de las señales
 * Simplemente enlaza el handler al struct de acciones de señal
 */
void setup_signals() {
  struct sigaction act;
  sigemptyset(&(act.sa_mask));

  /* Bloqueo señales durante ejecucion de handler */
  sigaddset(&(act.sa_mask), SIGALRM);
  sigaddset(&(act.sa_mask), SIGUSR1);
  sigaddset(&(act.sa_mask), SIGUSR2);

  act.sa_flags = 0;
  act.sa_handler = handler;

  if (sigaction(SIGALRM, &act, NULL) == ERR)
    die("sigaction SIGALRM");
  if (sigaction(SIGUSR1, &act, NULL) == ERR)
    die("sigaction SIGUSR1");
  if (sigaction(SIGUSR2, &act, NULL) == ERR)
    die("sigaction SIGUSR2");
}

/**
 * @brief Inicializa el temporizador del minero
 * Establece el manejador de señal y crea el temporizador de forma que al
 * terminar se envie SIGALARM. En caso de error termina el programa
 *
 * @param seconds Segundos de vida del minero
 * @param timer Puntero a objeto timer_t
 */
void miner_set_alarm(u64 seconds, timer_t *timer) {
  /* Aqui definimos el comportamiento del timer, queremos que mande SIGALARM */
  struct sigevent sevent;
  sevent.sigev_notify = SIGEV_SIGNAL; // Decimos que al terminar envie señal
  sevent.sigev_signo = SIGALRM;       // Aqui decimos cual de las señales enviar
  sevent.sigev_value.sival_ptr = timer; // "Adjuntamos" el timer al mensaje

  /* Esta ultima linea sirve para que si el OS envia varios sigalarm
   * simultaneamente, adjuntar el timer ayudara a que diferencie a que proceso
   * pertenece cada timer y asi detendra los procesos indicados */

  /* MONOTONIC es inmune a cambios de hora del sistema. Mas seguro */
  if (timer_create(CLOCK_MONOTONIC, &sevent, timer) == ERR)
    die("timer_create");

  struct itimerspec timer_spec;
  timer_spec.it_value.tv_sec = seconds;
  timer_spec.it_value.tv_nsec = 0;
  // No queremos que se repita
  timer_spec.it_interval.tv_sec = 0;
  timer_spec.it_interval.tv_nsec = 0;

  if (timer_settime(*timer, 0, &timer_spec, NULL) == ERR)
    die("timer_settime");
}

/**************************************************************************/
/*----------------------- IMPLEMENTACION FUNCIONES -----------------------*/
/**************************************************************************/

void *pow_seek(void *arg) {
  assert(arg != NULL);

  Arg_hilos *args = (Arg_hilos *)arg;

  if (*(args->found_value) == FOUND)
    return NULL;

  u64 i = 0;

  u64 *pow_result = (u64 *)malloc(sizeof(u64));
  if (pow_result == NULL)
    die("Error al reservar memoria para solucion de POW");

  for (i = args->min; i <= args->max; i++) {
    if (*(args->found_value) == FOUND)
      break;

    if (pow_hash(i) == args->target) {
      *(args->found_value) = FOUND;
      *pow_result = i;
      //      printf("Solution accepted: %08lu --> %08lu\n", args->target, i);
      return pow_result;
    }
  }

  free(pow_result);
  return NULL;
}

u64 calcular_solucion(u64 target, Miner_data *args) {
  /* VERIFICACION PARAMETROS DE ENTRADA */
  assert(args != NULL);

  /* INICIALIZO ARGUMENTOS DE POW_SEEK Y ARRAY DE HILOS  */
  pthread_t *hilos = (pthread_t *)malloc(args->n_threads * sizeof(pthread_t));
  if (hilos == NULL)
    die("Error al reservar memoria para hilos");

  Arg_hilos *thread_args =
      (Arg_hilos *)malloc(args->n_threads * sizeof(Arg_hilos));
  if (thread_args == NULL) {
    free(hilos);
    die("Error al reservar memoria para argumentos de hilos");
  }

  volatile int found_flag = 0;
  u64 rango_busqueda = POW_LIMIT / args->n_threads;

  /* CREACION DE HILOS */
  for (u64 i = 0; i < args->n_threads; i++) {
    thread_args[i].target = target;
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
      /* Con esto marcamos a los hilos que ya terminen */
      found_flag = 1;

      for (u64 j = 0; j < i; j++) {
        void *valor_retorno;
        pthread_join(hilos[j], &valor_retorno);

        /* pow_seek reserva memoria, hay que recoger la solucion y liberarla */
        if (valor_retorno != NULL)
          free(valor_retorno);
      }

      free(hilos);
      free(thread_args);

      die("Error al crear los hilos");
    }
  }

  u64 sol = 0;
  bool found = 0;

  for (u64 i = 0; i < args->n_threads; i++) {
    /*thread_return coge un void ** */
    void *valor_retorno;
    pthread_join(hilos[i], &valor_retorno);

    if (valor_retorno != NULL) {
      u64 *out = (u64 *)valor_retorno;
      sol = *out;
      found = true;

      free(out);
    }
  }

  /* LIBERACION DE MEMORIA */
  free(hilos);
  free(thread_args);
  // Casteamos a unsigned long int para prevenir warning
  return (found == 1) ? sol : (u64)ERR;
}

void minero(Miner_data *args, i32 *miner_pipe, i32 *logger_pipe,
            Miner_Mutexes *sems) {
  assert(args != NULL);
  assert(miner_pipe != NULL);
  assert(logger_pipe != NULL);

  setup_signals();

  /* ZONA CRITICA --- PROCESO APUNTA SU PID */
  sem_wait(sems->pid);
  pid_t foo[256];
  i32 n_active = get_active_pids_unlocked(PID_FILE, foo, -1, false);

  bool first_miner = (n_active <= 0);

  if (first_miner) {
    // Es el primer minero
    sem_wait(sems->tgt);
    if (write_target_unlocked(TARGET_FILE, TARGET_INIT) == ERR) {
      sem_post(sems->tgt);
      sem_post(sems->pid);
      die_msg("No se pudo escribir el target inicial");
    }
    sem_post(sems->tgt);
  }

  if (write_pid_unlocked(PID_FILE) == ERR) {
    sem_post(sems->pid);
    sem_close(sems->pid);
    die_msg("No se pudo escribir en PIDs.pid");
  }
  sem_post(sems->pid);

  if (first_miner) {
    i32 miner_count = 0;

    while (1) {
      sem_wait(sems->pid);
      /* El proceso se omite a si mismo, con esto evitamos un context switch
       * innecesario */
      miner_count = get_active_pids_unlocked(PID_FILE, foo, getpid(), false);
      sem_post(sems->pid);

      if (miner_count >= MIN_MINERS - 1) {
        break;
      }

      sleep(1);
    }

    for (i32 i = 0; i < miner_count; i++) {
      kill(foo[i], SIGUSR1);
    }

    /* De esta forma no hay que hacerse kill a uno mismo */
    start_mining = 1;
  } else {
    /* Bloqueamos sigusr1 y sigalrm, con esto en vez de pause, no perdemos las
     * señales que lleguen durante el bloqueo */
    sigset_t block_mask, old_mask, wait_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGUSR1);
    sigaddset(&block_mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &block_mask, &old_mask);

    /* Preparamos mascara de despertar al proceso (con sigusr1 o sigalrm) */
    wait_mask = old_mask;
    sigdelset(&wait_mask, SIGUSR1);
    sigdelset(&wait_mask, SIGALRM);

    /* El proceso duerme hasta que llegue la señal */
    while (!start_mining && !timeout)
      sigsuspend(&wait_mask);

    /* Restauramos mascara original */
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
  }

  start_mining = 0;

  /* Iniciamos el temporizador una vez comienza la mineria, no tendria sentido
   * iniciarlo sin siquiera haber suficientes mineros */
  timer_t m_timer;
  miner_set_alarm(args->time, &m_timer);

  u64 target = 0;
  Logger_args logger_args = {0};
  logger_args.winner = getpid();
  logger_args.validated = 1;

  while (!timeout) {
    sem_wait(sems->tgt);
    i32 target_read = read_target_unlocked(TARGET_FILE, &target);
    sem_post(sems->tgt);

    if (target_read == ERR)
      break;

    u64 sol = calcular_solucion(target, args);
    i32 status; // Esto solo sirve para el read, es temporal

    logger_args.target = target;
    logger_args.id = getpid();
    logger_args.solution = sol;
    logger_args.validated = (target == 0) ? 0 : 1;
    logger_args.votes = 0;
    logger_args.wallets = logger_args.winner;

    comunicar_logger(miner_pipe, &logger_args);

    if (read(logger_pipe[READ], &status, sizeof(i32)) <= 0)
      break;
  }

  timer_delete(m_timer);

  /* Mando señal de finalizacion */
  logger_args.target = (u64)ERR;
  comunicar_logger(miner_pipe, &logger_args);

  exit_network(PID_FILE, sems);
}
