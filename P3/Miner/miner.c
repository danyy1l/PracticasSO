/**
 * @file miner.c
 * @author Fernando Blanco & Danyyil Shykerynets
 * @brief Implementación de minero.
 * * Contiene la implementación de las funciones de cálculo del POW, así como la
 * lógica del miner implementación de las funciones de cálculo del POW, así como
 * la lógica del minero y el IPC entre minero y registrador
 * @version 2.0
 * @date 2026-04-01
 *
 * @copyright (c) 2026 Author. All Rights Reserved.
 */

#include "miner.h"
#include "logger.h"
#include "pow.h"
#include "shared.h"
#include "types.h"
#include "utils.h"
#include <asm-generic/errno-base.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <mqueue.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <string.h>

#define TARGET_INIT 0 /**< Valor de inicializacion del target */

/***********************************/
/*----- FUNCION AUXILIAR TEST -----*/
/***********************************/

/**
 * @brief Obtiene la hora actual en formato HH:MM:SS
 * @return Puntero a un string estatico con la hora
 */
char *get_time_str() {
  static char buffer[10];
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(buffer, sizeof(buffer), "%H:%M:%S", t);
  return buffer;
}

/***********************************/
/*------- AUXILIARES MINERO -------*/
/***********************************/

/**
 * @brief Manda un mensaje a logger
 * Función auxiliar para minero
 *
 * @param miner_pipe Tuberia de escritura del minero al registrador
 * @param args Estructura de argumentos del logger
 */
void comunicar_logger(i32 *miner_pipe, Logger_args *args);

/***********************************/
/*------------ SEÑALES ------------*/
/***********************************/

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
void handler(int sig);

/**
 * @brief Hace el setup de las señales
 * Simplemente enlaza el handler al struct de acciones de señal
 */
void setup_signals();

/**
 * @brief Inicializa el temporizador del minero
 * Establece el manejador de señal y crea el temporizador de forma que al
 * terminar se envie SIGALARM. En caso de error termina el programa
 *
 * @param seconds Segundos de vida del minero
 * @param timer Puntero a objeto timer_t
 */
void miner_set_alarm(u64 seconds, timer_t *timer);

/***********************************/
/*------- ESPERA INACTIVA ---------*/
/***********************************/

/**
 * @brief Espera a que se unan mas mineros a la red
 * En comentarios, tenemos la opcion de que la espera tambien se haga con un
 * maximo de intentos para prevenir que el proceso se quede en stall
 *
 * @param shared Estructura de memoria compartida entre mineros
 */
void wait_more_miners(SharedMinerData *shared);

/**
 * @brief Espera a la llegada de SIGUSR1 o SIGALRM de forma segura
 * Con sigsuspend y sigprocmask suspendemos el proceso de forma que si recibe
 * alguna señal durante la suspension no se pierda sino que ejecute su handler
 * una vez despierte
 */
void wait_signal(int sig, volatile sig_atomic_t *cond);

/***********************************/
/*------- FUNCIONES CALCULO -------*/
/***********************************/

/**
 * @brief Busca el valor objetivo en un rango dado
 * Funcion de calculo
 *
 * @param arg Puntero a lista de argumentos
 * @return Devuelve puntero al numero que consigue el valor objetivo
 */
void *pow_seek(void *arg);
/**
 * @brief Crea los hilos y separa la tarea
 *
 * @param target Objetivo de busqueda de la ronda
 * @param miner_data Estructura con informacion para minero (tiempo y numero de
 * hilos)
 * @return Devuelve la solucion para el POW con objetivo args->target
 */
u64 calcular_solucion(u64 target, Miner_data *args);

/* --- FUNCION AUXILIAR DE MEMORIA --- */

/* Función escudo contra interrupciones de señales */
void safe_sem_wait(sem_t *sem) {
  while (sem_wait(sem) == -1) {
    if (errno != EINTR) {
      perror("safe_sem_wait failed");
      exit(EXIT_FAILURE);
    }
    // Si fue EINTR , el bucle simplemente lo reintenta.
  }
}

void notify_all(SharedMinerData *shared, int signal) {
  safe_sem_wait(&shared->mutex);
  for (int i = 0; i < MAX_MINERS; i++) {
    if (shared->miners[i].miner_pid != 0 &&
        shared->miners[i].miner_pid != getpid()) {
      kill(shared->miners[i].miner_pid, signal);
    }
  }
  sem_post(&shared->mutex);
}

/********************************************************************/
/*--------------------- IMPLEMENTACION MINERO ----------------------*/
/********************************************************************/

void minero(Miner_data *args, i32 *miner_pipe, i32 *logger_pipe,
            SharedMinerData *shared, mqd_t mq) {
  assert(args != NULL);
  assert(miner_pipe != NULL);
  assert(logger_pipe != NULL);
  assert(shared != NULL);
  assert(mq != (mqd_t)ERR);

/* Cada minero pone una semilla aleatoria basada en el pid */
#ifdef FAKE
  srand(getpid() ^ time(NULL));
#endif /* ifdef FAKE */

  u64 target = 0;
  u32 round = 1;
  bool i_win = false;
  bool first_miner = false;
  bool release_win = false;
  u64 wallets = 0;

  setup_signals();

  /* ZONA CRITICA --- PROCESO APUNTA SU PID */
  safe_sem_wait(&shared->mutex);

  first_miner = (shared->active_miners == 1);

  if (first_miner)
    shared->miner_target = TARGET_INIT;

  sem_post(&shared->mutex);

  wait_more_miners(shared);

  /* Iniciamos el temporizador una vez comienza la mineria, no tendria
   * sentido iniciarlo sin siquiera haber suficientes mineros */
  timer_t m_timer;
  miner_set_alarm(args->time, &m_timer);

  while (!timeout) {
    wait_more_miners(shared);

    /* Caso ganador o primero minero */
    if (i_win || first_miner) {
      notify_all(shared, SIGUSR1);
      start_mining = 1;
      first_miner = false; // Solo para primera ronda
      /* Dejar unos ms para que el resto vuelvan de sigsuspend */
      // usleep(5000);
    } else {
      /* No soy el ganador ni el primero */
      wait_signal(SIGUSR1, &start_mining);
    }

    start_mining = 0;

    safe_sem_wait(&shared->mutex);
    target = shared->miner_target;
    sem_post(&shared->mutex);

    i_win = false;
    release_win = false;

    /* BUSQUEDA DE SOLUCION */
    u64 sol = calcular_solucion(target, args);

    if (sol != (u64)ERR && !start_voting && !timeout) {
      /* Check de si somos primeros */
      if (sem_trywait(&shared->win) == 0) {
        /* Caso victorioso: Somos los primeros */
        i_win = true;

#ifdef FAKE
        if (rand() % 100 < 10) {
          sol = 99999999; // Ponemos una solucion false con 0,1 de probabilidad
          printf("%d ha generado una solución falsa!\n", getpid());
        }
#endif /* ifdef FAKE */

        safe_sem_wait(&shared->mutex);

        for (int i = 0; i < MAX_MINERS; i++)
          shared->miners[i].current_vote = '\0';
        /* Se pone sol = target temporalmente para la votacion*/
        shared->miner_target = sol;

        sem_post(&shared->mutex);

        notify_all(shared, SIGUSR2);
        start_voting = 1;
      }
    }

    /* Votantes esperan a que ganador de el OK para votar */
    if (!i_win) {
      wait_signal(SIGUSR2, &start_voting);
    }

    /* VOTACION */
    if (start_voting && !timeout) {
      if (i_win) {
        /* El ganador espera a que todos voten y hace recuento.
         * Sondeo activo: comprueba votos hasta MAX_TRIES veces o timeout */
        u32 wait_iters = 0;
        /* Votos esperados: todos los mineros activos excepto yo */

        safe_sem_wait(&shared->mutex);
        u32 expected_votes =
            (shared->active_miners > 0) ? (u32)(shared->active_miners - 1) : 0;
        sem_post(&shared->mutex);

        while (wait_iters < MAX_TRIES && !timeout) {
          safe_sem_wait(&shared->mutex);
          u32 cur_votes = 0;
          for (int i = 0; i < MAX_MINERS; i++) {
            if (shared->miners[i].miner_pid != 0 &&
                shared->miners[i].miner_pid != getpid() &&
                shared->miners[i].current_vote != '\0')
              cur_votes++;
          }
          sem_post(&shared->mutex);
          if (cur_votes >= expected_votes)
            break;
          wait_iters++;
          usleep(100000); /* 100 ms por intento */
        }

        u32 positives = 0, total_votes = 0;
        safe_sem_wait(&shared->mutex);

        expected_votes =
            (shared->active_miners > 0) ? (u32)(shared->active_miners - 1) : 0;

        printf("Winner %d => [ ", getpid());
        for (int i = 0; i < MAX_MINERS; i++) {
          if (shared->miners[i].miner_pid != 0 &&
              shared->miners[i].miner_pid != getpid()) {

            char vote = shared->miners[i].current_vote;

            if (vote == 'Y')
              positives++;
            if (vote != '\0') {
              total_votes++;
              printf("%c ", vote);
            }
          }
        }

        u32 negatives = total_votes - positives;
        // Si no hay votantes acepta inmediatamente, si no, quedaria el sistema
        // en un bucle infinito de rechazo
        bool accepted = (expected_votes == 0) ||
                        ((positives > negatives) && (positives > 0));

        printf("] => %s\n", accepted ? "Accepted" : "Rejected");

        if (accepted) {
          wallets++;
          shared->miner_target = sol;
        } else {
          /* Si la solucion era erronea, recuperamos el anterior target */
          shared->miner_target = target;
        }

        sem_post(&shared->mutex);

        // Enviar a la cola del monitor
        MinerDataBlock mq_msg = {
            .target = target,
            .solution = sol,
            .round = round,
            .miner_pid = getpid(),
        };

        while (mq_send(mq, (char *)&mq_msg, sizeof(MinerDataBlock), 0) == ERR) {
          if (errno != EINTR) {
            perror("mq_send ronda");
            break;
          }
        }

        /* Registramos la ronda sea aceptada o no */
        Logger_args logger_args = {0};
        logger_args.winner = getpid();
        logger_args.id = round;
        logger_args.solution = sol;
        logger_args.target = target;
        logger_args.validated = accepted;
        logger_args.pos_votes = positives;
        logger_args.votes = total_votes;

        logger_args.wallets = wallets;

        comunicar_logger(miner_pipe, &logger_args);

        i32 status;
        if (read(logger_pipe[READ], &status, sizeof(i32)) <= 0)
          break;
      } else {
        /* El votante solo valida la solucion y escribe su voto */
        safe_sem_wait(&shared->mutex);
        u64 read_sol = shared->miner_target;

        char vote = pow_hash(read_sol) == target ? 'Y' : 'N';

        // Proceso apunta su voto
        for (int i = 0; i < MAX_MINERS; i++) {
          if (shared->miners[i].miner_pid == getpid()) {
            shared->miners[i].current_vote = vote;
            break;
          }
        }

        sem_post(&shared->mutex);
      }

      start_voting = 0;

      if (i_win) {
        sem_post(&shared->win);
        release_win = true;
        start_mining = 0;
      }
    }

    round++;
  }

  timer_delete(m_timer);

  /* Si muero habiendo ganado, el resto se quedan esperando y nunca continuan:
   * Mandamos SIGUSR1 para que continuen*/
  if (i_win && !release_win)
    sem_post(&shared->win);

  if (i_win)
    notify_all(shared, SIGUSR1);

  /* Mando señal de finalizacion */
  safe_sem_wait(&shared->mutex);

  bool is_last = false;
  for (int i = 0; i < MAX_MINERS; i++) {
    if (shared->miners[i].miner_pid == getpid()) {
      shared->miners[i].miner_pid = 0;
      shared->active_miners--;
      is_last = (shared->active_miners == 0);
      break;
    }
  }

  sem_post(&shared->mutex);

  if (is_last) {
    MinerDataBlock msg_salida;
    memset(&msg_salida, 0, sizeof(MinerDataBlock));
    msg_salida.target = (u64)-1;

    while (mq_send(mq, (char *)&msg_salida, sizeof(MinerDataBlock), 0) == ERR) {
      if (errno != EINTR) {
        perror("mq_send fin");
        break;
      }
    }
  }
  Logger_args logger_args = {0};
  logger_args.target = (u64)ERR;
  comunicar_logger(miner_pipe, &logger_args);
}

void comunicar_logger(i32 *miner_pipe, Logger_args *args) {
  /* No es necesario comprobar argumentos dado que vienen de funcion minero */
  write(miner_pipe[WRITE], args, sizeof(Logger_args));
}

void handler(int sig) {
  switch (sig) {
  case SIGINT:
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

  sigset_t full_block;
  sigemptyset(&full_block);
  sigaddset(&full_block, SIGUSR1);
  sigaddset(&full_block, SIGUSR2);
  // NOTA: NO bloqueamos SIGALRM ni SIGINT porque queremos que puedan
  // interrumpir el pow_seek
  if (sigprocmask(SIG_BLOCK, &full_block, NULL) == ERR)
    die("sigprocmask full");
}

void miner_set_alarm(u64 seconds, timer_t *timer) {
  assert(timer != NULL);

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

void wait_more_miners(SharedMinerData *shared) {
  while (!timeout) {
    safe_sem_wait(&shared->mutex);
    u64 count = shared->active_miners;
    sem_post(&shared->mutex);

    if (count >= MIN_MINERS)
      break;
    sleep(1);
  }
}

void wait_signal(int sig, volatile sig_atomic_t *cond) {
  /* Bloqueamos sig y sigalrm, con esto en vez de pause, no perdemos
   * las señales que lleguen durante el bloqueo */
  sigset_t block_mask, old_mask, wait_mask;
  sigemptyset(&block_mask);
  sigaddset(&block_mask, sig);
  sigaddset(&block_mask, SIGALRM);
  sigprocmask(SIG_BLOCK, &block_mask, &old_mask);

  /* Preparamos mascara de despertar al proceso (con sigusr1 o sigalrm) */
  wait_mask = old_mask;
  sigdelset(&wait_mask, sig);
  sigdelset(&wait_mask, SIGALRM);

  /* El proceso duerme hasta que llegue la señal */
  while (!(*cond) && !timeout) {
    sigsuspend(&wait_mask);
  }

  /* Cambio aqui: No hace falta restaurar porque ya lo hace sigsuspend */
  /* SI HACE FALTA, sin esto el ultimo minero ignora sigalrm y no termina, por
   * tanto, tampoco termina el monitor */
  sigprocmask(SIG_SETMASK, &old_mask, NULL);
}

void *pow_seek(void *arg) {
  assert(arg != NULL);

  Arg_hilos *args = (Arg_hilos *)arg;

  if (*(args->found_value) == FOUND)
    return NULL;

  u64 i = 0;

  for (i = args->min; i <= args->max; i++) {
    /* Ahora detenemos la busqueda si termina o toca votar */
    if (*(args->found_value) == FOUND || start_voting || timeout)
      break;

    if (pow_hash(i) == args->target) {
      *(args->found_value) = FOUND;
      u64 *pow_result = (u64 *)malloc(sizeof(u64));
      if (pow_result == NULL)
        die("Error al reservar memoria para solucion de POW");
      *pow_result = i;
      // printf("Solution accepted: %08lu --> %08lu\n", args->target, i);
      return pow_result;
    }
  }

  return NULL;
}

/**
 * @brief Crea los hilos y separa la tarea
 *
 * @param target Objetivo de busqueda de la ronda
 * @param miner_data Estructura con informacion para minero (tiempo y numero de
 * hilos)
 * @return Devuelve la solucion para el POW con objetivo args->target
 */
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
    /* Con el ternario aseguro que en el ultimo rango de todos llegue hasta
     * el limite, podria quedarse corto */
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

        /* pow_seek reserva memoria, hay que recoger la solucion y liberarla
         */
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
