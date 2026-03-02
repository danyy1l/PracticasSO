#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  char *argv[3] = {"ls", "./", NULL};
  pid_t pid;

  pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    if (execl("/usr/bin/ls", argv[0], argv[1], argv[2])) {
      perror("execvp");
      exit(EXIT_FAILURE);
    }
  } else {
    wait(NULL);
  }
  exit(EXIT_SUCCESS);
}
