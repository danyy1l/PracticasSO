#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  FILE *pf = NULL;

  pf = fopen("name", "r");
  if (pf == NULL) {
    printf("%d\n", errno);
    perror("");
  }

  pf = fopen("/etc/shadow", "r");
  if (pf == NULL) {
    printf("%d\n", errno);
    perror("");
  }

  return EXIT_SUCCESS;
}
