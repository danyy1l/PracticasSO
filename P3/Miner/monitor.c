/**
 * @file monitor.c
 * @author Danyyil Shykerynets & Fernando Blanco
 * @brief Punto de entrada del programa
 * * Contiene el main a ejecutar del programa
 * @version 1.0
 * @date 2026-04-28
 */

#include "monitor_core.h"
#include "shared.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  u64 lag_comprobador, lag_monitor;
  pid_t child_pid;
  int exit_status = EXIT_SUCCESS;

  SharedMinerData *miner_shm;
  SharedMonitorData *monitor_shm;
  mqd_t miner_mq = (mqd_t)ERR;

  /* CONTROL DE ARGUMENTOS */
  if (argc != 3) {
    fprintf(stderr, "Monitor exited unexpectedly!\n");
    die_msg("Usage: ./monitor <LAG_COMPROBADOR> <LAG_MONITOR>");
  }

  lag_comprobador = str_to_u64(argv[1]);
  lag_monitor = str_to_u64(argv[2]);

  setup_interrupt();

  /* SHARED MEMORY */
  /* Miner - Comprobador */
  miner_shm = create_miner_shm();
  if (miner_shm == NULL) {
    fprintf(stderr, "No se pudo crear shm de mineros\n");
    exit_status = EXIT_FAILURE;
    goto cleanup_miner;
  }

  /* Comprobador - Monitor */
  monitor_shm = create_monitor_shm();
  if (monitor_shm == NULL) {
    fprintf(stderr, "No se pudo crear shm de monitor\n");
    exit_status = EXIT_FAILURE;
    goto cleanup_monitor;
  }

  /* MESSAGE QUEUE */
  /* Miner - Comprobador */
  miner_mq = create_miner_queue();
  if (miner_mq == (mqd_t)ERR) {
    fprintf(stderr, "No se pudo crear message queue\n");
    exit_status = EXIT_FAILURE;
    goto cleanup_queue;
  }

  /* MAIN LOOP */
  child_pid = fork();

  if (child_pid == ERR) {
    perror("fork_monitor");
    exit_status = EXIT_FAILURE;
  } else if (child_pid == 0) {
    /* PROCESO MONITOR */
    monitor(lag_monitor, monitor_shm);
    detach_miner_shm(miner_shm);
    exit(EXIT_SUCCESS);
  } else {
    /* PROCESO COMPROBADOR */
    comprobador(lag_comprobador, miner_shm, monitor_shm, miner_mq);
    printf("Comprobador exited successfully\n");
    waitpid(child_pid, NULL, 0);
  }

cleanup_queue:
  if (miner_mq != (mqd_t)ERR)
    destroy_miner_queue(miner_mq);
cleanup_monitor:
  if (monitor_shm != NULL)
    destroy_monitor_shm(monitor_shm);
cleanup_miner:
  if (miner_shm != NULL)
    destroy_miner_shm(miner_shm);

  return exit_status;
}
