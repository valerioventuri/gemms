#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv) {

  sigset_t signal_mask;

  if(argc<2) {
    fprintf(stderr,"Usage: %s <command> <arguments>\n",argv[0]);
    exit(1);
  }

  /* Check if command exists and is executable */
  if(access(argv[1], X_OK)) {
    perror("access");
    fprintf(stderr,"Error: command %s not found or cannot be executed\n", argv[1]);
    exit(1);
  }

  /* Unblock SIGTERM */
  sigemptyset (&signal_mask);
  sigaddset (&signal_mask, SIGTERM);
  if(pthread_sigmask(SIG_UNBLOCK, &signal_mask, NULL)) {
    perror("pthread_sigmask");
    fprintf(stderr,"Error: cannot unblock SIGTERM\n");
    exit(1);
  }

  if(execv(argv[1], &argv[1])) {
    perror("execv");
    fprintf(stderr,"Error: cannot execute command %s\n",argv[1]);
    exit(1);
  }

  return 0;
}
