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
  char poolName[GPFS_MAXNAMLEN];
  struct mntent *ent;
  FILE *mnttab;

  if(argc!=2) {
    fprintf(stderr,"Usage: yamssShowPoolOccupancy gpfs_device\n");
    exit(1);
  }
  snprintf(fsdev,256,"/dev/%s",argv[1]);

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

  for(i=0; i<nPools; i++) {

    if(gpfs_getpoolname(ent->mnt_dir, statBuf[i].f_poolid, poolName, GPFS_MAXNAMLEN)) {
      fprintf(stderr, "gpfs_getpoolname failed, %s\n", strerror(errno));
      free(statBuf);
      exit(1);
    }

    printf("%s %lld %lld %lld %lld\n", poolName, statBuf[i].f_blocks*statBuf[i].f_bsize/1024, statBuf[i].f_bavail*statBuf[i].f_bsize/1024, statBuf[i].f_mblocks*statBuf[i].f_bsize/1024, statBuf[i].f_mfree*statBuf[i].f_bsize/1024);

  }

  free(statBuf);

  return 0;

}


