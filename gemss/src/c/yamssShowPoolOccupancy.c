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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <gpfs.h>

#include <mntent.h>
#include <fstab.h>

int main(int argc, char **argv) {

  char fsdev[256];
  unsigned int poolId;
  int nPools, statSize;
  gpfs_statfspool_t *statBuf;
  int i;
  char poolName[GPFS_MAXNAMLEN], max_poolName[GPFS_MAXNAMLEN];
  struct mntent *ent;
  FILE *mnttab;
  int flag;
  long long max_avail;

  if(argc==1||(argc==2&&!strcmp(argv[1],"-f"))||(argc==3&&strcmp(argv[1],"-f"))||argc>3) {
    fprintf(stderr,"Usage: yamssShowPoolOccupancy [-f] gpfs_device\n");
    exit(1);
  }
  flag=!strcmp(argv[1],"-f")?1:0;  

  snprintf(fsdev,256,"/dev/%s",flag?argv[2]:argv[1]);

  /* look for the mount point where the file resides */
  mnttab = fopen("/etc/mtab", "r");
  ent=getmntent(mnttab);
  while(ent) {
    if(!strcmp(fsdev,ent->mnt_fsname)) break;
    ent = getmntent(mnttab);
  }
  fclose(mnttab);
  if(!ent) {
    fprintf(stderr,"cannot determine mountpoint for device %s\n",fsdev);
    exit(1);
  }

  nPools=0;
  if(gpfs_statfspool(ent->mnt_dir, &poolId, 0, &nPools, NULL, 0)) {
    fprintf(stderr, "gpfs_statfspool failed, %s\n", strerror(errno));
    exit(1);
  }

  statSize=sizeof(gpfs_statfspool_t)*nPools;
  statBuf=malloc(statSize);

  if(gpfs_statfspool(ent->mnt_dir, &poolId, 0, &nPools, statBuf, statSize)) {
    fprintf(stderr, "gpfs_statfspool failed, %s\n", strerror(errno));
    free(statBuf);
    exit(1);
  }

  max_avail=-1;
  max_poolName[0]='\0';
  for(i=0; i<nPools; i++) {

    if(gpfs_getpoolname(ent->mnt_dir, statBuf[i].f_poolid, poolName, GPFS_MAXNAMLEN)) {
      fprintf(stderr, "gpfs_getpoolname failed, %s\n", strerror(errno));
      free(statBuf);
      exit(1);
    }

    if(flag) {
      if(statBuf[i].f_bavail>max_avail) {
        max_avail=statBuf[i].f_bavail;
        strcpy(max_poolName,poolName);
      }
    } else {
      printf("%s %lld %lld %lld %lld\n", poolName, statBuf[i].f_blocks*statBuf[i].f_bsize/1024, statBuf[i].f_bavail*statBuf[i].f_bsize/1024, statBuf[i].f_mblocks*statBuf[i].f_bsize/1024, statBuf[i].f_mfree*statBuf[i].f_bsize/1024);
    }
  }

  free(statBuf);

  if(flag) {
    if(max_avail<0) {
      fprintf(stderr, "Command failed in an impossible way\n");
      exit(1);
    } else {
      printf("%s\n",max_poolName);
    }
  }

  return 0;

}


