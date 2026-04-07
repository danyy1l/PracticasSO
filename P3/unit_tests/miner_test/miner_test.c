/**
 * @brief Testea el modulo Miner
 *
 * @file miner_test.c
 * @author Danyyil Shykerynets
 * @version 1.0
 * @date 01-03-2026
 * @Copyright (c) 2026 Author. All Rights Reserved.
 */

#include "miner_test.h"
#include "files.h"
#include "miner.h"
#include "pow.h"
#include "test.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_TESTS 5

extern volatile sig_atomic_t timeout;
extern volatile sig_atomic_t start_voting;

int main(int argc, char **argv) {
  int test = 0, all = 1;

  if (argc < 2) {
    printf("Running all test for module Miner (Sync & Concurrency):\n");
  } else {
    test = atoi(argv[1]);
    all = 0;
    printf("Running test %d:\t", test);
    if (test < 1 || test > MAX_TESTS) {
      printf("Error: unknown test %d\t", test);
      exit(EXIT_SUCCESS);
    }
  }

  if (all || test == 1)
    test1_calcular_solucion();
  if (all || test == 2)
    test1_wait_votes_success();
  if (all || test == 3)
    test2_wait_votes_timeout();
  if (all || test == 4)
    test1_wait_more_miners_success();
  if (all || test == 5)
    test2_wait_more_miners_timeout();

  PRINT_PASSED_PERCENTAGE;
  return EXIT_SUCCESS;
}

void test1_open_pipes() {
  int fd_A[2] = {0};
  int fd_B[2] = {0};
  open_pipes(fd_A, fd_B);
  PRINT_TEST_RESULT(fd_A[READ] != 0 && fd_B[WRITE] != 0);
}
void test2_open_pipes() {
  int fd_A[2] = {0};
  int fd_B[2] = {0};
  open_pipes(fd_A, fd_B);
  PRINT_TEST_RESULT(fd_B[READ] != 0 && fd_B[WRITE] != 0);
}

void test1_calcular_solucion() {
  Miner_data args = {0};
  args.n_threads = 4;
  args.time = 10;

  u64 expected_sol = 12345;
  u64 target = pow_hash(expected_sol);

  start_voting = 0;
  timeout = 0;

  u64 sol = calcular_solucion(target, &args);
  PRINT_TEST_RESULT(sol == expected_sol);
}

void test1_wait_votes_success() {
  Miner_Mutexes sems;
  initialize_mutexes(&sems);
  timeout = 0;

  FILE *f = fopen(VOTES_FILE, "w");
  if (f) {
    fputs("YYN", f);
    fclose(f);
  }

  wait_votes(3, &sems);
  PRINT_TEST_RESULT(1);

  unlink(VOTES_FILE);
  close_mutexes(&sems);
  sem_unlink(PID_MUTEX);
  sem_unlink(TARGET_MUTEX);
  sem_unlink(VOTES_MUTEX);
  sem_unlink(WINNER_MUTEX);
}

void test2_wait_votes_timeout() {
  Miner_Mutexes sems;
  initialize_mutexes(&sems);

  /* Simulamos que la alarma del sistema ha sonado */
  timeout = 1;

  /* Le pedimos 100 votos aunque el archivo no exista*/
  wait_votes(100, &sems);
  PRINT_TEST_RESULT(1);

  timeout = 0; /* Limpiamos flag */
  close_mutexes(&sems);
  sem_unlink(PID_MUTEX);
  sem_unlink(TARGET_MUTEX);
  sem_unlink(VOTES_MUTEX);
  sem_unlink(WINNER_MUTEX);
}

void test1_wait_more_miners_success() {
  Miner_Mutexes sems;
  initialize_mutexes(&sems);
  timeout = 0;

  pid_t dummy_miner = fork();
  if (dummy_miner == 0) {
    close_mutexes(&sems);

    sleep(2);
    exit(0);
  } else {
    FILE *f = fopen(PID_FILE, "w");
    if (f) {
      fprintf(f, "%d\n%d\n", getpid(), dummy_miner);
      fclose(f);
    }

    wait_more_miners(&sems);
    PRINT_TEST_RESULT(1);

    waitpid(dummy_miner, NULL, 0);
    unlink(PID_FILE);
    close_mutexes(&sems);
    sem_unlink(PID_MUTEX);
    sem_unlink(TARGET_MUTEX);
    sem_unlink(VOTES_MUTEX);
    sem_unlink(WINNER_MUTEX);
  }
}

void test2_wait_more_miners_timeout() {
  Miner_Mutexes sems;
  initialize_mutexes(&sems);

  FILE *f = fopen(PID_FILE, "w");
  if (f) {
    fprintf(f, "%d\n", getpid());
    fclose(f);
  }

  /* Forzamos el timeout */
  timeout = 1;

  wait_more_miners(&sems);
  PRINT_TEST_RESULT(1);

  timeout = 0; /* Limpiamos flag */
  unlink(PID_FILE);
  close_mutexes(&sems);
  sem_unlink(PID_MUTEX);
  sem_unlink(TARGET_MUTEX);
  sem_unlink(VOTES_MUTEX);
  sem_unlink(WINNER_MUTEX);
}
