#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void logger() { printf("I am Logger!\n"); }

int main(int argc, char *argv[]) {
  logger();
  return EXIT_SUCCESS;
}
