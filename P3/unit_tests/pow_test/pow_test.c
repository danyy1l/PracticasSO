/**
 * @brief Testea el modulo pow
 *
 * @file pow_test.c
 * @author Danyyil Shykerynets
 * @version 1.0
 * @date 01-03-2026
 * @Copyright (c) 2026 Author. All Rights Reserved.
 */

#include "pow_test.h"
#include "pow.h"
#include "test.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_TESTS 3

/**
 * @brief Main function for POW unit tests.
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
    printf("Running all test for module Pow:\n");
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
    test1_pow();
  if (all || test == 2)
    test2_pow();
  if (all || test == 3)
    test3_pow();
  PRINT_PASSED_PERCENTAGE;

  return 1;
}

void test1_pow() {
  u64 x = 1;
  u64 fx = pow_hash(x);
  PRINT_TEST_RESULT(fx == 5803690);
}

void test2_pow() {
  u64 x = 0;
  u64 fx = pow_hash(x);
  PRINT_TEST_RESULT(fx == 24849);
}

void test3_pow() {
  u64 x = -1;
  u64 fx = pow_hash(x);
  PRINT_TEST_RESULT(fx == pow_hash(18446744073709551615));
}
