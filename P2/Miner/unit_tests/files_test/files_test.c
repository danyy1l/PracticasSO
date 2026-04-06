/**
 * @brief Testea el modulo de archivos
 *
 * @file files_test.c
 * @author Danyyil Shykerynets & Fernando Blanco
 * @version 2.0
 * @date 2026-04-06
 */

#include "files_test.h"
#include "files.h"
#include "test.h"
#include "types.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TEST_PID_FILE "test_pids.pid"
#define TEST_VOTES_FILE "test_votes.vot"
#define MAX_TESTS 5

int main(int argc, char **argv) {
  int test = 0, all = 1;
  if (argc > 1) {
    test = atoi(argv[1]);
    all = 0;
  }

  if (argc < 2) {
    printf("Running all test for module Files/Consensus:\n");
  } else {
    printf("Running test %d:\t", test);
    if (test < 1 || test > MAX_TESTS) {
      printf("Error: unknown test %d\t", test);
      exit(EXIT_SUCCESS);
    }
  }

  if (all || test == 1)
    test1_open_pipes();
  if (all || test == 2)
    test2_close_pipes();
  if (all || test == 3)
    test3_mutex_init();
  if (all || test == 4)
    test4_pids_io();
  if (all || test == 5)
    test5_voting_system();

  PRINT_PASSED_PERCENTAGE;
  return EXIT_SUCCESS;
}

/* ========================================================= */
/* ================== IMPLEMENTACION TESTS ================= */
/* ========================================================= */

void test1_open_pipes() {
  int miner_pipe[2] = {-1, -1};
  int logger_pipe[2] = {-1, -1};

  open_pipes(miner_pipe, logger_pipe);

  /* Comprobamos que el SO nos ha dado descriptores válidos (>= 0) */
  PRINT_TEST_RESULT(miner_pipe[READ] >= 0 && miner_pipe[WRITE] >= 0 &&
                    logger_pipe[READ] >= 0 && logger_pipe[WRITE] >= 0);

  /* Limpiamos para el siguiente test */
  close_pipes(miner_pipe, logger_pipe);
}

void test2_close_pipes() {
  int miner_pipe[2] = {-1, -1};
  int logger_pipe[2] = {-1, -1};

  open_pipes(miner_pipe, logger_pipe);
  close_pipes(miner_pipe, logger_pipe);

  /* Comprobamos que tu función resetea los valores a -1 por seguridad */
  PRINT_TEST_RESULT(miner_pipe[READ] == -1 && miner_pipe[WRITE] == -1 &&
                    logger_pipe[READ] == -1 && logger_pipe[WRITE] == -1);
}

void test3_mutex_init() {
  Miner_Mutexes sems;
  initialize_mutexes(&sems);

  PRINT_TEST_RESULT(sems.pid != SEM_FAILED && sems.tgt != SEM_FAILED &&
                    sems.vot != SEM_FAILED && sems.win != SEM_FAILED);

  close_mutexes(&sems);
  sem_unlink(PID_MUTEX);
  sem_unlink(TARGET_MUTEX);
  sem_unlink(VOTES_MUTEX);
  sem_unlink(WINNER_MUTEX);
}

void test4_pids_io() {
  write_pid_unlocked(TEST_PID_FILE);

  pid_t active[10];
  i32 count = get_active_pids_unlocked(TEST_PID_FILE, active, -1, false);

  PRINT_TEST_RESULT(count == 1 && active[0] == getpid());

  unlink(TEST_PID_FILE);
}

void test5_voting_system() {
  write_vote(TEST_VOTES_FILE, 'Y');
  write_vote(TEST_VOTES_FILE, 'Y');
  write_vote(TEST_VOTES_FILE, 'N');

  u32 pos = 0, total = 0;
  bool accepted = count_votes(TEST_VOTES_FILE, getpid(), &pos, &total);

  /* Debe ser aceptado (2 a favor, 1 en contra) y contar 3 totales */
  PRINT_TEST_RESULT(accepted == true && pos == 2 && total == 3);

  unlink(TEST_VOTES_FILE);
}
