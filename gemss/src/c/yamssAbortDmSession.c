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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <gpfs.h>
#include <dmapi.h>

int main(int argc, char **argv) {

   dm_sessid_t sid = DM_NO_SESSION;

   if (argc<2) {
     fprintf(stderr, "%s: missing arguments <sid>\n", argv[0]);
     exit(1);
   }

   sscanf(argv[1],"%0llx",&sid);

   if(dm_destroy_session(sid)) {
      fprintf(stderr,"dm_destroy_session: failed, %s\n", strerror(errno));
      return 1;
   }

   return 0;
}

