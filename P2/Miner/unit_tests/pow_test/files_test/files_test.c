/**
 * @brief Testea el modulo de archivos
 *
 * @file files_test.c
 * @author Danyyil Shykerynets & Fernando Blanco
 * @version 1.0
 * @date 2026-04-04
 */

#include "files_test.h"
#include "files.h"
#include "test.h"
#include "types.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_TESTS 4

#define TEST_TARGET "test_target.dat"
#define TEST_VOTES "test_votes.dat"

int main(int argc, char **argv) {
  int test = 0;
  int all = 1;

  if (argc < 2) {
    printf("Running all test for module Files/Consensus:\n");
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
    test1_mutex_init();
  if (all || test == 2)
    test1_target_io();
  if (all || test == 3)
    test1_voting_accepted();
  if (all || test == 4)
    test2_voting_rejected();

  PRINT_PASSED_PERCENTAGE;

  return EXIT_SUCCESS;
}

/* ========================================================= */
/* ================== IMPLEMENTACION TESTS ================= */
/* ========================================================= */

void test1_mutex_init() {
  Miner_Mutexes sems;

  /* Inicializamos semaforos */
  initialize_mutexes(&sems);

  /* Comprobamos que ninguno haya fallado */
  PRINT_TEST_RESULT(sems.pid != SEM_FAILED && sems.tgt != SEM_FAILED &&
                    sems.vot != SEM_FAILED && sems.win != SEM_FAILED);

  close_mutexes(&sems);
  sem_unlink(PID_MUTEX);
  sem_unlink(TARGET_MUTEX);
  sem_unlink(VOTES_MUTEX);
  sem_unlink(WINNER_MUTEX);
}

void test1_target_io() {
  u64 test_val = 987654321;
  u64 read_val = 0;

  write_target_unlocked(TEST_TARGET, test_val);
  i32 res = read_target_unlocked(TEST_TARGET, &read_val);

  PRINT_TEST_RESULT(res == OK && read_val == test_val);

  unlink(TEST_TARGET); /* Borramos archivo de prueba */
}

void test1_voting_accepted() {
  /* Simulamos un archivo de votos con 3 SI y 1 NO */
  FILE *f = fopen(TEST_VOTES, "w");
  if (f) {
    fputc('Y', f);
    fputc('Y', f);
    fputc('N', f);
    fputc('Y', f);
    fclose(f);
  }

  u32 pos = 0, total = 0;
  bool accepted = count_votes(TEST_VOTES, getpid(), &pos, &total);

  /* Debe ser aceptado, con 3 positivos y 4 totales */
  PRINT_TEST_RESULT(accepted == true && pos == 3 && total == 4);

  unlink(TEST_VOTES);
}

void test2_voting_rejected() {
  FILE *f = fopen(TEST_VOTES, "w");
  if (f) {
    fputc('N', f);
    fputc('N', f);
    fclose(f);
  }

  u32 pos = 0, total = 0;
  bool accepted = count_votes(TEST_VOTES, getpid(), &pos, &total);

  PRINT_TEST_RESULT(accepted == false && pos == 0 && total == 2);

  unlink(TEST_VOTES);
}
