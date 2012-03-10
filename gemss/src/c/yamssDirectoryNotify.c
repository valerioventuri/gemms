/*
############################################################################
# Copyright 2008-2012 Istituto Nazionale di Fisica Nucleare
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

#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile int event_fd;
static volatile int event_sig;
static volatile void *event_data;

static void handler(int sig, siginfo_t *si, void *data) {
  event_fd = si->si_fd;
  event_sig = sig;
  event_data = data;

  exit(0);
}

int main(int argc, char **argv) {
  struct sigaction act;
  int fd;

  if(argc!=3) {
    fprintf(stderr,"Usage: yamssDirNotify DIRECTORY SLEEP_TIME\n");
    exit(0);
  }

  act.sa_sigaction = handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGRTMIN+1, &act, NULL);

  fd = open(argv[1], O_RDONLY);
  if(fd<0) {
    fprintf(stderr,"yamssDirNotify: directory %s does not exist\n",argv[1]);
    exit(0);    
  }

  fcntl(fd, F_SETSIG, SIGRTMIN + 1);
  fcntl(fd, F_NOTIFY, DN_ACCESS|DN_MODIFY|DN_CREATE|DN_RENAME|DN_DELETE|DN_ATTRIB|DN_MULTISHOT);

  sleep(atoi(argv[2]));

  return 1;

}

