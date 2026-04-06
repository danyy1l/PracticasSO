/**
 * @file miner.c
 * @author Fernando Blanco & Danyyil Shykerynets
 * @brief Implementación de minero.
 * * Contiene la implementación de las funciones de cálculo del POW, así como la
 * lógica del miner implementación de las funciones de cálculo del POW, así como
 * la lógica del minero y el IPC entre minero y registrador
 * @version 2.0
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
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TARGET_INIT 0 /**< Valor de inicializacion del target */

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

/**
 * @brief El minero abandona la red
 * Abandona la red, libera la memoria dedicada a los semaforos por el proceso, y
 * si es el ultimo, elimina todas las referencias a los semaforos para que no
 * queden activos en shm
 *
 * @param filename Fichero con PIDs de toda la red
 * @param sems Estructura de semaforos del proyecto
 */
void exit_network(const char *filename, Miner_Mutexes *sems);

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
 * @param sems Estructura de semaforos del sistema
 */
void wait_more_miners(Miner_Mutexes *sems);

/**
 * @brief Espera a la llegada de SIGUSR1 o SIGALRM de forma segura
 * Con sigsuspend y sigprocmask suspendemos el proceso de forma que si recibe
 * alguna señal durante la suspension no se pierda sino que ejecute su handler
 * una vez despierte
 */
void wait_signal(int sig, volatile sig_atomic_t *cond);

/**
 * @brief Hace espera inactiva mientras los procesos que no han ganado votan
 *
 * @param votes Numero de votos esperados
 * @param sems Semaforos del sistema
 */
void wait_votes(u32 votes, Miner_Mutexes *sems);

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

/********************************************************************/
/*--------------------- IMPLEMENTACION MINERO ----------------------*/
/********************************************************************/

void minero(Miner_data *args, i32 *miner_pipe, i32 *logger_pipe,
            Miner_Mutexes *sems) {
  assert(args != NULL);
  assert(miner_pipe != NULL);
  assert(logger_pipe != NULL);
  assert(sems != NULL);

/* Cada minero pone una semilla aleatoria basada en el pid */
#ifdef FAKE
  srand(getpid() ^ time(NULL));
#endif /* ifdef FAKE */

  setup_signals();

  /* ZONA CRITICA --- PROCESO APUNTA SU PID */
  sem_wait(sems->pid);
  if (write_pid_unlocked(PID_FILE) == ERR) {
    sem_post(sems->pid);
    close_mutexes(sems);
    die_msg("No se pudo escribir en PIDs.pid");
  }

  pid_t foo[MAX_MINERS];
  i32 n_active = get_active_pids_unlocked(PID_FILE, foo, -1, false);
  if (n_active == ERR) {
    sem_post(sems->pid);
    close_mutexes(sems);
    die_msg("miner.c - No se pudo leer PIDs");
  }

  bool first_miner = (n_active <= 1);

  if (first_miner) {
    // Es el primer minero
    sem_wait(sems->tgt);
    if (write_target_unlocked(TARGET_FILE, TARGET_INIT) == ERR) {
      sem_post(sems->tgt);
      sem_post(sems->pid);
      close_mutexes(sems);
      die_msg("No se pudo escribir el target inicial");
    }
    sem_post(sems->tgt);
  }

  sem_post(sems->pid);

  /* Impresion al unirse un minero */
  printf("Miner %d added to system\n\n", getpid());
  printf("===== ACTIVE MINERS =====\n");
  for (i32 i = 0; i < n_active; i++)
    printf("- Process %7d\n", foo[i]);

  printf("\n");

  wait_more_miners(sems);

  if (first_miner) {
    printf("Starting mining!\n\n");
  }

  /* Iniciamos el temporizador una vez comienza la mineria, no tendria
   * sentido iniciarlo sin siquiera haber suficientes mineros */
  timer_t m_timer;
  miner_set_alarm(args->time, &m_timer);

  u64 target = 0;
  u32 round = 1;
  bool i_win = false;

  u64 wallets = 0;

  bool release_win = false;

  while (!timeout) {
    wait_more_miners(sems);

    sem_wait(sems->pid);
    i32 miner_count = get_active_pids_unlocked(PID_FILE, foo, getpid(), false);

    /* Caso ganador o primero minero */
    if (i_win || first_miner) {
      /* Habria que enviar la señal mientras el fichero de pids esta locked para
       * que no se actualice a mitad */
      for (i32 i = 0; i < miner_count; i++)
        kill(foo[i], SIGUSR1);

      start_mining = 1;
      first_miner = false; // Solo para primera ronda
      sem_post(sems->pid);
      /* Dejar unos ms para que el resto vuelvan de sigsuspend */
      // usleep(5000);
    } else {
      sem_post(sems->pid);
      /* No soy el ganador ni el primero */
      wait_signal(SIGUSR1, &start_mining);
    }

    start_mining = 0;

    sem_wait(sems->tgt);
    i32 target_read = read_target_unlocked(TARGET_FILE, &target);
    sem_post(sems->tgt);

    if (target_read == ERR)
      break;

    i_win = false;
    release_win = false;

    /* BUSQUEDA DE SOLUCION */
    u64 sol = calcular_solucion(target, args);

    if (sol != (u64)ERR && !start_voting && !timeout) {
      /* Check de si somos primeros */
      if (sem_trywait(sems->win) == 0) {
        /* Caso victorioso: Somos los primeros */
        i_win = true;

#ifdef FAKE
        if (rand() % 100 < 10) {
          sol = 99999999; // Ponemos una solucion false con 0,1 de probabilidad
        }
#endif /* ifdef FAKE */

        /* Limpiamos el archivo de votos */
        sem_wait(sems->vot);
        FILE *fp = fopen(VOTES_FILE, "w");
        if (fp)
          fclose(fp);
        sem_post(sems->vot);

        sem_wait(sems->tgt);
        write_target_unlocked(TARGET_FILE, sol);
        sem_post(sems->tgt);

        for (i32 i = 0; i < miner_count; i++)
          kill(foo[i], SIGUSR2);

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
        /* El ganador espera a que todos voten y hace recuento */
        wait_votes(miner_count, sems);

        u32 positives;
        u32 total_votes;
        sem_wait(sems->vot);
        bool accepted =
            count_votes(VOTES_FILE, getpid(), &positives, &total_votes);
        sem_post(sems->vot);

        if (accepted) {
          wallets++;

          sem_wait(sems->tgt);
          write_target_unlocked(TARGET_FILE, sol);
          sem_post(sems->tgt);

        } else {
          /* Si la solucion era erronea, recuperamos el anterior target */
          sem_wait(sems->tgt);
          write_target_unlocked(TARGET_FILE, target);
          sem_post(sems->tgt);
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
        u64 read_sol;
        sem_wait(sems->tgt);
        read_target_unlocked(TARGET_FILE, &read_sol);
        sem_post(sems->tgt);

        char is_valid = pow_hash(read_sol) == target ? 'Y' : 'N';

        sem_wait(sems->vot);
        write_vote(VOTES_FILE, is_valid);
        sem_post(sems->vot);
      }

      start_voting = 0;

      if (i_win) {
        sem_post(sems->win);
        release_win = true;
        start_mining = 0;
      }
    }

    round++;
  }

  timer_delete(m_timer);

  /* Si muero habiendo ganado, el resto se quedan esperando y nunca continuan:
   * Mandamos SIGUSR1 para que continuen*/
  if (i_win) {
    pid_t all_pids[MAX_MINERS];

    sem_wait(sems->pid);
    i32 active_pids =
        get_active_pids_unlocked(PID_FILE, all_pids, getpid(), false);
    sem_post(sems->pid);

    for (i32 i = 0; i < active_pids; i++)
      kill(all_pids[i], SIGUSR1);

    if (!release_win)
      sem_post(sems->win);
  }

  /* Mando señal de finalizacion */
  Logger_args logger_args = {0};
  logger_args.target = (u64)ERR;
  comunicar_logger(miner_pipe, &logger_args);

  exit_network(PID_FILE, sems);
}

void comunicar_logger(i32 *miner_pipe, Logger_args *args) {
  /* No es necesario comprobar argumentos dado que vienen de funcion minero */
  write(miner_pipe[WRITE], args, sizeof(Logger_args));
}

void exit_network(const char *filename, Miner_Mutexes *sems) {
  assert(filename != NULL);
  assert(sems != NULL);

  pid_t active_miners[MAX_MINERS];

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
  close_mutexes(sems);
}

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

void wait_more_miners(Miner_Mutexes *sems) {
  pid_t foo[MAX_MINERS];
  i32 miner_count = 0;
  // u32 tries = 0;

  while (!timeout) {
    sem_wait(sems->pid);
    miner_count = get_active_pids_unlocked(PID_FILE, foo, -1, false);
    sem_post(sems->pid);
    if (miner_count == ERR) {
      close_mutexes(sems);
      die_msg("miner.c - No se pudieron leer PIDs");
    }

    if (miner_count >= MIN_MINERS)
      break;

    /*
    if( tries == MAX_TRIES ){
      close_mutexes(sems);
      die_msg("Waited too long for more miners. Killing system...")
    }
    */

    //++tries;
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

  /* Restauramos mascara original */
  if (sigprocmask(SIG_SETMASK, &old_mask, NULL) == ERR)
    die("sigprocmask");
}

void wait_votes(u32 votes, Miner_Mutexes *sems) {
  u32 tries = 0;
  long current_votes = 0;

  /* Hacemos sondeo, comprobamos hasta maximo de intentos, timeout o que esten
   * todos */
  while (current_votes < votes && tries < MAX_TRIES && !timeout) {

    sem_wait(sems->vot);
    FILE *f = fopen(VOTES_FILE, "r");
    if (f != NULL) {
      fseek(f, 0, SEEK_END);
      current_votes = ftell(f); // El tamaño en bytes = numero de letras
      fclose(f);
    }
    sem_post(sems->vot);

    if (current_votes < votes) {
      tries++;
      usleep(100000);
    }
  }
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
