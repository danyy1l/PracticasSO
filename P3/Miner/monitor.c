/**
 * @file monitor.c
 * @author Danyyil Shykerynets
 * @brief Punto de entrada del programa
 * * Contiene el main a ejecutar del programa
 * @version 1.0
 * @date 2026-04-28
 */

#include "file_utils.h"
#include "logger.h"
#include "miner.h"
#include "types.h"
#include "shared.h"
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <mqueue.h>

/***********************************/
/*------------ SEÑALES ------------*/
/***********************************/
/**
 * Flag de stop del monitor
 * Mientras sea 0, el monitor trabaja, en cuanto se reciba
 * SIGINT, se pone a 1 y el monitor abandona la red
 */
static volatile sig_atomic_t stop = 0;

int main(int argc, char *argv[]) {
  u64 lag_comprobador, lag_monitor;
  pid_t pid;
  SharedMinerData *shared_miner_data;
  SharedMonitorData *shared_monitor_data;
  mqd_t miner_queue;

  /* CONTROL DE ARGUMENTOS */
  if (argc != 3) {
    fprintf(stderr, "Monitor exited unexpectedly!\n");
    die_msg("Usage: ./monitor <LAG_COMPROBADOR> <LAG_MONITOR>");
  }

  lag_comprobador = str_to_u64(argv[1]);
  lag_monitor = str_to_u64(argv[2]);

  /* SHARED MEMORY */
  /* Miner - Comprobador */
  shared_miner_data = create_miner_shm();
  /* Comprobador - Monitor */
  shared_monitor_data = create_monitor_shm();

  /* MESSAGE QUEUE */
  /* Miner - Comprobador */
  miner_queue = create_miner_queue();

  /* HANDLER */

  /* MAIN LOOP */
  pid = fork();
  if (pid == ERR) {
    /* Libera recursos */
    //------>
    /**/
    die("fork monitor");
  }

  if (pid == 0) {
    /* PROCESO COMPROBADOR */
    comprobador(lag_comprobador);
  } else {
    /* PROCESO MONITOR */
    monitor(lag_monitor);
  }

  printf("Hola");
}

void comprobador(u64 lag_comprobador) {
  while (!stop) {
    /* Espera el lag del comprobador (ms)*/
    usleep(lag_comprobador * 1000);
    /* Comprueba si hay un ganador*/
    
    /**/
    /* Si hay ganador, comparte info con el monitor*/
    //Down ( sem_empty ) ;
    //Down ( sem_mutex ) ;
    //AñadirElemento () ;
    //Up ( sem_mutex ) ;
    //Up ( sem_fill ) ;
    /**/
  }
}

void monitor(u64 lag_monitor) {
  while (!stop) {
    /* Espera el lag del monitor (ms)*/
    usleep(lag_monitor * 1000);
    /* Comprueba el estado del sistema*/
    //Down ( sem_fill ) ;

    //Down ( sem_mutex ) ;
    //ExtraerElemento () ;
    //Up ( sem_mutex ) ;
    //Up ( sem_empty ) ;
    /* Imprime información sobre el estado del sistema*/
  }
}

/**
 * @brief Manejador de señales
 * Coloca el flag de stop a 1, indicando que el monitor ha terminado
 *
 * @param sig Numero de la señal a manejar (SIGALARM, SIGUSR1 o SIGUSR2)
 */
void handler(int sig) {
  switch (sig) {
  case SIGINT:
    stop = 1;
    break;
  }
}
