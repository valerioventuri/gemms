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

#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <mntent.h>
#include <fstab.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <gpfs.h>

#define NOBODY_UID 99
#define NOBODY_GID 99

/* max time waiting for stubbifying one file is 1x100=100 s */
#define SLEEP_TIME 1
#define MAX_LOOP 100

int main(int argc,char **argv) {

  char *yamssStubbifyPath="/system/YAMSS_STUBBIFY";

  struct stat64 filebuf;
  char s[4096];
  char hname[4096];
  int fd;
  char rpath[4096];
  int i, j;
  struct statvfs vbuf, sbuf;
  struct mntent *ent;
  FILE *mnttab;
  uid_t uid, euid;
  gid_t gid, egid;
  char s_uid[64];
  char s_euid[64];
  char s_gid[64];
  char s_egid[64];

  char *tmpflist[argc];
  char *origflist[argc];
  int iflist=0;
  int rc=0;

  char at[16];

  if(argc==1) {
    fprintf(stderr,"Usage: yamssStubbify FILE...\n");
    exit(1);
  }

  /* get user credentials */
  uid=getuid();
  euid=geteuid();
  gid=getgid();
  egid=getegid();
  sprintf(s_uid,"%d ",uid);
  sprintf(s_euid,"%d ",euid);
  sprintf(s_gid,"%d ",gid);
  sprintf(s_egid,"%d ",egid);

  /* get host name */
  if(gethostname(hname,4096)) {
    fprintf(stderr,"yamssStubbify: Error: cannot get hostname\n");
    return -1;
  }

  for(i=1; i<argc; i++) {

    /* reset effective uid and gid */
    if(setegid(gid)<0) {
      perror("setegid");
      fprintf(stderr, "yamssStubbify: Error: cannot reset gid %d\n", egid);
      return 1;
    }
    if(seteuid(uid)<0) {
      perror("seteuid");
      fprintf(stderr, "yamssStubbify: Error: cannot reset uid %d\n", euid);
      return 1;
    }

    if(!realpath(argv[i],rpath)) {
      perror("realpath");
      fprintf(stderr,"yamssStubbify: Error while calling realpath for file %s\n",argv[i]);
      rc=1;
      continue;
    }

    if(access(argv[i],W_OK)) {
      perror("access");
      fprintf(stderr,"yamssStubbify: you must have write permission on file %s to stubbify it\n",argv[i]);
      rc=1;
      continue;
    }

    /* check if file is pinned */
    if(getxattr(rpath, "user.storm.pinned", at, 10)>0) {
      at[10]=0;
      fprintf(stderr,"yamssStubbify: file %s is pinned on disk and cannot be stubbified\n",rpath);
      rc=1;
      continue;
    }

    /* check if file is migrated */
    if(getxattr(rpath, "user.storm.migrated", at, 10)<0) {
      fprintf(stderr,"yamssStubbify: file %s does not appear to have been migrated to tape and cannot be stubbified\n",rpath);
      rc=1;
      continue;
    }

    if(statvfs(rpath, &vbuf)) {
      perror("statvfs");
      fprintf(stderr,"yamssStubbify: Error: cannot statvfs file %s\n",rpath);
      rc=1;
      continue;
    }

    /* look for the mount point where the file resides */
    mnttab = fopen("/etc/mtab", "r");
    ent=getmntent(mnttab);
    while(ent) {
      if(!statvfs(ent->mnt_dir, &sbuf)) {
        if(vbuf.f_fsid==sbuf.f_fsid) {
          break;
        }
      }
      ent = getmntent(mnttab);
    }
    fclose(mnttab);

    if(!ent) {
      fprintf(stderr,"yamssStubbify: Error: cannot find mountpoint for file %s\n",rpath);
      rc=1;
      continue;
    }

    /* set effective uid and gid to nobody */
    if(setegid(NOBODY_GID)<0) {
      perror("setegid");
      fprintf(stderr, "yamssStubbify: Error: cannot set gid to nobody\n");
      return 1;
    }
    if(seteuid(NOBODY_UID)<0) {
      perror("seteuid");
      fprintf(stderr, "yamssStubbify: Error: cannot set uid to nobody\n");
      return 1;
    }

    /* generate temporary file with unique name containing the file to be recalled */
    sprintf(s,"%s%s/%s.%d.XXXXXXXX",ent->mnt_dir,yamssStubbifyPath,hname,getpid());
    if((fd=mkstemp(s))<0) {
      fprintf(stderr,"yamssStubbify: Error: cannot generate temporary file %s for file %s\n",s,rpath);
      rc=1;
      continue;    
    }

    /* write user, group and file name into temporary file */
    if(write(fd,s_uid,strlen(s_uid))<0 || write(fd,s_euid,strlen(s_euid))<0 ||
       write(fd,s_gid,strlen(s_gid))<0 || write(fd,s_egid,strlen(s_egid))<0 ||
       write(fd,rpath,strlen(rpath))<0 || write(fd,"\n",1)<0) {

      perror("write");
      fprintf(stderr,"yamssStubbify: Error: cannot write into temporary file %s for file %s\n",s,rpath);
      close(fd);
      if(unlink(s)<0) {
        perror("unlink");
        fprintf(stderr,"yamssStubbify: Error: cannot delete temporary file %s for file %s\n",s,rpath);
      }
      rc=1;
      continue;
    }
    close(fd);

    /* save temporary and real file names for later use */
    tmpflist[iflist]=malloc(strlen(s)+1);
    origflist[iflist]=malloc(strlen(rpath)+1);
    strcpy(tmpflist[iflist],s);
    strcpy(origflist[iflist++],rpath);

  }

  /* reset effective uid and gid */
  if(setegid(gid)<0) {
    perror("setegid");
    fprintf(stderr, "yamssStubbify: Error: cannot reset gid %d\n", egid);
    return 1;
  }
  if(seteuid(uid)<0) {
    perror("seteuid");
    fprintf(stderr, "yamssStubbify: Error: cannot reset uid %d\n", euid);
    return 1;
  }


  for(i=0; i<iflist; i++) {
    printf("Freeing disk space for file %s ... ",origflist[i]);
    for(j=0; j<MAX_LOOP; j++) {
      if(access(tmpflist[i], F_OK)) {
        if(!gpfs_stat(origflist[i], &filebuf) && filebuf.st_blocks==0) {
          printf("success\n");
        } else {
          printf("failure\n");
          rc=1;
        }
        break;
      }

      /* sleep a bit between one check and the other */
      sleep(SLEEP_TIME);
    }
  }

  return rc;
}
