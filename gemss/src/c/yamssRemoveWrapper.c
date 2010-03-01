#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc,char **argv) {

  char *end;
  int uid, gid;

  if(argc!=4) {
    fprintf(stderr,"Usage: yamssRemoveWrapper UID GID FILE\n");
    exit(1);
  }

  uid = strtoul(argv[1], &end, 10);
  if(*end!='\0') {
    fprintf(stderr, "yamssRemoveWrapper: Error: uid %s for file %s is not a number\n", argv[1],argv[3]);
    exit(1);
  }

  gid = strtoul(argv[2], &end, 10);
  if(*end!='\0') {
    fprintf(stderr, "yamssRemoveWrapper: Error: gid %s for file %s is not a number\n", argv[2],argv[3]);
    exit(1);
  }

  /* set effective uid and gid */
  if(setegid(gid)<0) {
    perror("setegid");
    fprintf(stderr, "yamssRemoveWrapper: Error: cannot set gid %s for file %s\n", argv[2],argv[3]);
    exit(1);
  }
  if(seteuid(uid)<0) {
    perror("seteuid");
    fprintf(stderr, "yamssRemoveWrapper: Error: cannot set uid %s for file %s\n", argv[1],argv[3]);
    exit(1);
  }


  /* remove the file */
  if(unlink(argv[3])<0) {
    perror("unlink");
    fprintf(stderr,"yamssRemoveWrapper: Error: cannot remove file %s as uid %s and gid %s\n",argv[3],argv[1],argv[2]);
    exit(1);
  }

  return 0;
}
