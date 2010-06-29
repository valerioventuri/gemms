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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <mntent.h>
#include <fstab.h>

#define NOBODY_UID 99
#define NOBODY_GID 99

int main(int argc,char **argv) {

  char *yamssPreloadPath="/system/YAMSS_PRELOAD";

  char s[4096];
  char hname[4096];
  int fd;
  char rpath[4096];


  if(argc!=4) {
    fprintf(stderr,"I/O error: recall wrapper must be called passing file name, pid of the calling process and mount point\n");
    exit(1);
  }

  /* get host name */
  if(gethostname(hname,4096)) {
    fprintf(stderr,"I/O error: something weird, cannot get hostname for file %s\n",argv[1]);
    return -1;
  }

  if(!realpath(argv[1],rpath)) {
    perror("realpath");
    fprintf(stderr,"I/O error: cannot convert %s to an absolute path\n",argv[1]);
    return -1;
  }


  /* generate temporary file with unique name containing the file to be recalled */
  sprintf(s,"%s%s/%s.%s.XXXXXXXX",argv[3],yamssPreloadPath,hname,argv[2]);
  if((fd=mkstemp(s))<0) {
    fprintf(stderr,"I/O error: something weird, cannot generate temporary file %s for file %s\n",s,argv[1]);
    return -1;    
  }

  /* write file name to recall into temporary file */
  if(write(fd,rpath,strlen(rpath))<0) {
    perror("write");
    fprintf(stderr,"I/O error: cannot write into temporary file %s for file %s\n",s,rpath);
    close(fd);
    if(unlink(s)<0) {
      perror("unlink");
      fprintf(stderr,"I/O error: cannot delete temporary file %s for file %s\n",s,rpath);
    }
    return -1;
  }

  close(fd);

  return 0;
}
