/**
 * @file files.c
 * @author Danyyil Shykerynets
 * @brief Implementacion de manejo de archivos.
 * * Implementa las funciones necesarias para lectura y escritura en los
 * ficheros del proyecto.
 * @version 1.0
 * @date 2026-04-02
 *
 * @copyright (c) 2026 Author. All Rights Reserved.
 */

#include "files.h"
#include "types.h"
#include <assert.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

void initialize_mutexes(Miner_Mutexes *sems) {
  sems->pid = sem_open(PID_MUTEX, O_CREAT, S_IRUSR | S_IWUSR, 1);
  sems->tgt = sem_open(TARGET_MUTEX, O_CREAT, S_IRUSR | S_IWUSR, 1);
  sems->vot = sem_open(VOTES_MUTEX, O_CREAT, S_IRUSR | S_IWUSR, 1);
  sems->win = sem_open(WINNER_MUTEX, O_CREAT, S_IRUSR | S_IWUSR, 1);

  if (sems->pid == SEM_FAILED || sems->tgt == SEM_FAILED ||
      sems->vot == SEM_FAILED || sems->win == SEM_FAILED)
    die("sem_open");
}

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

/* Esta funcion podria simplemente retornar el numero, pero en caso de error, al
 * ser unsigned no podemos definir un codigo de error especifico, ya que todos
 * los numeros serian objetivos validos. Tal vez (u64)ERR pero si quisieramos
 * extender POW_LIMIT no valdria */
i32 read_target_unlocked(const char *filename, u64 *out_target) {
  if (filename == NULL || out_target == NULL)
    return ERR;

  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    return ERR;
  }

  if (fscanf(fp, "%lu", out_target) != 1) {
    fclose(fp);
    return ERR;
  }

  fclose(fp);

  return OK;
}

i32 write_target_unlocked(const char *filename, u64 target) {
  if (filename == NULL)
    return ERR;

  FILE *fp = fopen(filename, "w");
  if (fp == NULL) {
    return ERR;
  }

  if (fprintf(fp, "%lu\n", target) < 0) {
    fclose(fp);
    return ERR;
  }

  fclose(fp);
  return OK;
}

i32 get_active_pids_unlocked(const char *filename, pid_t *active_miners,
                             pid_t removed_pid, bool cleanup) {
  pid_t read_pid;
  u32 pid_counter = 0;

  FILE *fp = NULL;
  // Si limpiamos abrimos con permiso de escritura, si no, solo lectura
  if ((fp = fopen(filename, cleanup ? "r+" : "r")) == NULL)
    return ERR;

  /* Esto funciona porque el write se hace con \n, si no, fgets y sscanf */
  while (fscanf(fp, "%d", &read_pid) == 1) {
    if (read_pid == removed_pid)
      continue;

    /* Mandamos señal de hangup para revisar si el proceso esta activo, si lo
     * esta guardamos */
    if (kill(read_pid, 0) == 0) {
      active_miners[pid_counter++] = read_pid;
    }
  }

  /* Limpiamos el archivo y reescribimos los pids activos */
  if (cleanup == true) {
    ftruncate(fileno(fp), 0);
    rewind(fp);

    for (u32 i = 0; i < pid_counter; i++) {
      fprintf(fp, "%d\n", active_miners[i]);
    }
  }

  fclose(fp);
  return pid_counter;
}

void write_vote(const char *filename, char vote) {
  if (filename == NULL)
    die("write_vote");

  FILE *fp = fopen(filename, "a");
  if (fp == NULL)
    die("fopen voting");

  fputc(vote, fp);

  fclose(fp);
}

bool count_votes(const char *filename, pid_t winner_pid, u32 *out_positives) {
  u32 positives = 0, negatives = 0;

  FILE *fp = fopen(filename, "r");
  if (fp == NULL)
    die("fopen votes");

  printf("Winner %d => [ ", winner_pid);

  int character;
  while ((character = fgetc(fp)) != EOF) {
    if (character == 'Y') {
      positives++;
      printf("Y ");
    } else if (character == 'N') {
      negatives++;
      printf("N ");
    }
  }

  fclose(fp);

  bool accepted = (positives >= negatives);

  *out_positives = positives;

  printf("] => %s\n", accepted ? "Accepted" : "Rejected");

  /* Limpiamos el archivo de votaciones */
  fp = fopen(VOTES_FILE, "w");
  if (fp != NULL)
    fclose(fp);

  return accepted;
}
