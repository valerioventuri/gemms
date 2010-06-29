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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <mntent.h>
#include <fstab.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <attr/xattr.h>
#define __USE_GNU
#include <dlfcn.h>
#undef __USE_GNU 
#include <gpfs.h>
#include <stdarg.h>

#define SLEEP_TIME 10  /* sleep time betwenn checks of recall status (in seconds) */
#define RECALL_TIMEOUT 172800 /* timeout for giving up a recall with an error status (in seconds) */

int __yamssPreloadOpen__(char *fname) {

  char *yamssRecallWrapper="/usr/local/yamss/preload/bin/yamssRecallWrapper"; /* absolute path of recall wrapper command */

  char *yamssConfigPath="/system/YAMSS_CONFIG";

  struct statfs fsbuf;
  struct stat filebuf;
  struct statvfs vbuf, sbuf;
  struct mntent *ent;
  FILE *mnttab;

  char s[2048];
  char at[16];
  time_t rectime, now, errdate;

  /* check if filesystem is GPFS */
  if(statfs(fname, &fsbuf)) return 0;
  if(fsbuf.f_type!=GPFS_SUPER_MAGIC) return 0;
  /* it's a GPFS! Go ahead */

  /* check if file is on disk */
  if(stat(fname, &filebuf)) return 0;

  /* We discover the file locality by checking the number of 
     blocks allocated on disk. This works if files do not have holes... */
  if((filebuf.st_blocks*512)>=filebuf.st_size) return 0;
  /* File is migrated, maybe... go ahead */

  if(statvfs(fname, &vbuf)) {
    perror("statvfs");
    fprintf(stderr,"I/O error: cannot statvfs file %s\n",fname);
    return -1;
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
    fprintf(stderr,"I/O error: something weird, cannot find mountpoint for file %s\n",fname);
    return -1;
  }

  /* Check file name lenght, for the moment statically limited */
  if((strlen(yamssRecallWrapper)+strlen(fname)+2+6)>2048) {
    fprintf(stderr,"I/O error: file name %s too long\n",fname);
    return -1;
  }

  sprintf(s,"%s%s",ent->mnt_dir,yamssConfigPath);
  if(access(s, F_OK)) {
    /* if yamss config cannot be accessed, we assume that the file system is not yamss-managed */
    return 0;
  }

  /* Check if recall wrapper command exists and is executable */
  if(access(yamssRecallWrapper, X_OK)) {
    perror("access");
    fprintf(stderr,"I/O error: yamssRecallWrapper command not found for file %s\n",fname);
    return -1;
  }

  /* Execute recall wrapper command */
  sprintf(s,"%s %s %d %s",yamssRecallWrapper, fname, getpid(), ent->mnt_dir);
  if(system(s)) {
    fprintf(stderr,"I/O error: cannot execute yamssRecallWrapper command for file %s\n",fname);
    return -1;
  }

  /* save current time, i.e. time when recall was submitted */
  rectime=time(NULL);

  /* Poll until file is on disk */
  while(1) {
    if(stat(fname, &filebuf)) {
      perror(fname);
      return -1;
    }
    if((filebuf.st_blocks*512)>=filebuf.st_size) {
      /* File has been recalled */
      sleep(SLEEP_TIME);
      break;
    }

    sleep(SLEEP_TIME);

    /* check if an error condition is set on the file after recall submission time */
    if(getxattr(fname, "user.TSMErrD", at, 16)>0) {
      /* not sure it is needed to enforce null termination */
      at[10]=0;

      /* File has an error condition set */

      errdate=strtol(at,NULL,10);

      if(rectime<errdate) {      
        fprintf(stderr,"I/O error: cannot recall file %s to disk\n",fname);
        return -1;
      }
    }

    /* check if recall timeout expired */
    now=time(NULL);
    if((now-rectime)>RECALL_TIMEOUT) {
      fprintf(stderr,"I/O error: timed out - cannot recall file %s on disk after %d seconds\n",fname, RECALL_TIMEOUT);
      return -1;
    }
	
  }

  /* everything is OK */
  return 0;
}

int (*__real_open__)(const char *, int, ...)=NULL;
int (*__real_open64__)(const char *, int, ...)=NULL;

int open(const char *pathname, int flags, ...) {
  if (!__real_open__) __real_open__ = dlsym(RTLD_NEXT, "open");

  if(!__yamssPreloadOpen__((char*)pathname)) {
    mode_t mode;
    va_list ap;

    va_start(ap, flags);
    mode = (mode_t)va_arg(ap, mode_t);
    va_end(ap);

    return __real_open__(pathname, flags, mode);
  }

  return -1;
}

int open64(const char *pathname, int flags, ...) {
  if (!__real_open64__) __real_open64__ = dlsym(RTLD_NEXT, "open64");

  if(!__yamssPreloadOpen__((char*)pathname)) {
    mode_t mode;
    va_list ap;

    va_start(ap, flags);
    mode = (mode_t)va_arg(ap, mode_t);
    va_end(ap);

    return __real_open64__(pathname, flags, mode);
  }

  return -1;
}

#ifdef _LARGE_FILE_SOURCE

FILE *(*__real_fopen64__)(const char *, const char *)=NULL;
FILE *(*__real_freopen64__)(const char *, const char *, FILE *)=NULL;

FILE *fopen64(const char *pathname, const char *mode) {
  if (!__real_fopen64__) __real_fopen64__ = dlsym(RTLD_NEXT, "fopen64");
  if(!__yamssPreloadOpen__((char*)pathname)) return __real_fopen64__(pathname, mode);
  return NULL;
}


FILE *freopen64(const char *pathname, const char *mode, FILE *stream) {
  if (!__real_freopen64__) __real_freopen64__ = dlsym(RTLD_NEXT, "freopen64");
  if(!__yamssPreloadOpen__((char*)pathname)) return __real_freopen64__(pathname, mode, stream);
  return NULL;
}

#else

static FILE *(*__real_fopen__)(const char *, const char *)=NULL;
static FILE *(*__real_freopen__)(const char *, const char *, FILE *)=NULL;

FILE *fopen(const char *pathname, const char *mode) {
  if (!__real_fopen__) __real_fopen__ = dlsym(RTLD_NEXT, "fopen");
  if(!__yamssPreloadOpen__((char*)pathname)) return __real_fopen__(pathname, mode);
  return NULL;
}


FILE *freopen(const char *pathname, const char *mode, FILE *stream) {
  if (!__real_freopen__) __real_freopen__ = dlsym(RTLD_NEXT, "freopen");
  if(!__yamssPreloadOpen__((char*)pathname)) return __real_freopen__(pathname, mode, stream);
  return NULL;
}

#endif
