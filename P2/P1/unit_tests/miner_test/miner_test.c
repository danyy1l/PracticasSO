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
#include "miner.h"
#include "pow.h"
#include "test.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_TESTS 5

/**
 * @brief Main function for Miner unit tests.
 *
 * You may execute ALL or a SINGLE test
 *   1.- No parameter -> ALL test are executed
 *   2.- A number means a particular test (the one identified by that number)
 *       is executed
 *
 */
int main(int argc, char **argv) {

  int test = 0;
  int all = 1;

  if (argc < 2) {
    printf("Running all test for module Miner:\n");
  } else {
    test = atoi(argv[1]);
    all = 0;
    printf("Running test %d:\t", test);
    if (test < 1 && test > MAX_TESTS) {
      printf("Error: unknown test %d\t", test);
      exit(EXIT_SUCCESS);
    }
  }

  if (all || test == 1)
    test1_open_pipes();
  if (all || test == 2)
    test2_open_pipes();
  if (all || test == 3)
    test1_pow_seek();
  if (all || test == 4)
    test2_pow_seek();
  if (all || test == 5)
    test3_pow_seek();
  PRINT_PASSED_PERCENTAGE;

  return 1;
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

void test1_pow_seek() {
  Arg_hilos *x = (Arg_hilos *)malloc(sizeof(Arg_hilos));
  u64 *sol = NULL;
  volatile int found_flag = 0;
  x->found_value = &found_flag;
  x->target = 24849;
  x->min = 0;
  x->max = POW_LIMIT - 1;
  sol = (u64 *)pow_seek(x);
  PRINT_TEST_RESULT(sol != NULL && *sol == 0);
  free(sol);
  free(x);
}

void test2_pow_seek() {
  Arg_hilos *x = (Arg_hilos *)malloc(sizeof(Arg_hilos));
  u64 *sol = NULL;
  volatile int found_flag = 1;
  x->found_value = &found_flag;
  x->target = 0;
  x->min = 0;
  x->max = POW_LIMIT - 1;
  sol = (u64 *)pow_seek(x);
  PRINT_TEST_RESULT(sol == NULL);
  free(sol);
  free(x);
}

void test3_pow_seek() {
  Arg_hilos *x = (Arg_hilos *)malloc(sizeof(Arg_hilos));
  u64 *sol = NULL;
  volatile int found_flag = 0;
  x->found_value = &found_flag;
  x->target = 0;
  x->min = 0;
  x->max = 1;
  sol = (u64 *)pow_seek(x);
  PRINT_TEST_RESULT(sol == NULL);
  free(sol);
  free(x);
}
