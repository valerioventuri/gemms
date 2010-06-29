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

#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#define __USE_GNU
#include <dlfcn.h>
#undef __USE_GNU

/* Hook: preload of these functions allows to bypass dsmmigrate check of st_blocks and restore IBMObj DMAPI EA */

int (*__real_lxstat__)(int ver, const char *path, struct stat *buf)=NULL;
int (*__real_lxstat64__)(int ver, const char *path, struct stat64 *buf)=NULL;

int __lxstat(int ver, const char *file_name, struct stat *buf) {
  if (!__real_lxstat__) __real_lxstat__ = dlsym(RTLD_NEXT, "__lxstat");
  int ret=__real_lxstat__(ver, file_name, buf);
  if (buf->st_blocks == 0) {
    char *val=getenv("YAMSS_PRELOAD_STAT_FILE_NAME");
    if(val == NULL || strcmp(val,file_name)) return ret;
    buf->st_blocks = 1;
  }
  return ret;
}

int __lxstat64(int ver, const char *file_name, struct stat64 *buf) {
  if (!__real_lxstat64__) __real_lxstat64__ = dlsym(RTLD_NEXT, "__lxstat64");
  int ret=__real_lxstat64__(ver, file_name, buf);
  if (buf->st_blocks == 0) {
    char *val=getenv("YAMSS_PRELOAD_STAT_FILE_NAME");
    if(val == NULL || strcmp(val,file_name)) return ret;
    buf->st_blocks = 1;
  }
  return ret;
}


