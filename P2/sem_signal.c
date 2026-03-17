#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <errno.h>

#define SEM_NAME "/example_sem"

void handler(int sig) { return; }

int main(void) {
  sem_t *sem = NULL;
  struct sigaction act;

  if ((sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) ==
      SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }

  sigemptyset(&(act.sa_mask));
  act.sa_flags = 0;

  /* The handler for SIGINT is set. */
  act.sa_handler = handler;
  if (sigaction(SIGINT, &act, NULL) < 0) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  printf("Starting wait (PID=%d)\n", getpid());
  /* Bucle para reintentar si la llamada es interrumpida por una señal */
  while (sem_wait(sem) == -1) {
    if (errno == EINTR) {
      /* Fue interrumpido por una señal capturada (ej. SIGINT).
         El bucle iterará y volverá a llamar a sem_wait. */
      continue;
    } else {
      /* Ocurrió un error distinto y crítico (ej. semáforo inválido) */
      perror("sem_wait falló");
      exit(EXIT_FAILURE);
    }
  }

  printf("Finishing wait\n");
  printf("Finishing wait\n");
  sem_close(sem);
  sem_unlink(SEM_NAME);
}
