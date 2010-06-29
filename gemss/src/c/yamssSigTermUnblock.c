/*
############################################################################
# Copyright 2008-2010 Istituto Nazionale di Fisica Nucleare
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
############################################################################
*/

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
