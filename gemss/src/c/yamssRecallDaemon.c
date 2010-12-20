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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>

#include <dmapi.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/statvfs.h>
#include <sys/vfs.h>
#include <mntent.h>

#include <time.h>

#include <gpfs.h>
#include <sys/stat.h>

// Maximum number of recalls that can be served simultaneously
#define MAXIMUM_ALLOWED_CHILDREN 1000

#define HANDLE_LEN 64
#define ALL_AVAIL_MSGS  0
#define DMAPI_SESSION_NAME "yamRecD"
#define DMAPI_YAMSS_SLEEP_TIME 5

#define EXTOBJID_LEN 28

// sleep time betwenn checks of recall status (in seconds) 
#define RECALL_CHECK_SLEEP_TIME 5
// timeout for giving up a recall with an error status (in seconds)
#define RECALL_TIMEOUT 86400

extern char *optarg;
extern int  optind, optopt, opterr;
extern int  errno;
char *Progname;
int Verbose;
dm_sessid_t sid = DM_NO_SESSION;
unsigned int child_proc_count = 0;

void setup_dmapi();
void find_session();

void token_recovery(int);

void event_loop();
void set_events();
void register_mount();
void spawn_child(dm_token_t, void*, size_t, char *);
void exit_program(int);
void usage(char *);
void setup_signals();
void child_handler(int);

void finalize_init();

int filesystem_is_mounted(void);

void block_signals(pid_t);

void usage(char *prog) {
  fprintf(stderr, "Usage: %s ", prog);
  fprintf(stderr, " <-v verbose> ");
  fprintf(stderr, "filesystem \n");
}

void *fs_hanp;
size_t fs_hlen;
char *fsname;

int global_state;

int main(int argc, char **argv) {	
  int c;

  // disable buffering for stdout and stderr streams
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  // print out pid (for logger)
  printf("%d\n",getpid());

  Progname  = argv[0];
  fsname  = NULL;

  while ((c = getopt(argc, argv, "v")) != EOF) {
    switch (c) {
    case 'v':
      Verbose = 1;
      break;
    case '?':
      default:
      usage(Progname);
      exit(1);
    }
  }
  if (optind >= argc) {
    usage(Progname);
    exit(1);
  }
  fsname = argv[optind];
  if (fsname == NULL) {
    usage(Progname);
    exit(1);
  }

  // reset global state
  global_state=1;

  printf("Starting up yamssRecallDaemon\n");

  // setup various signal handlers
  printf("Installing signal handlers\n");
  setup_signals();	
  
  // initialize dmap
  printf("Initializing DMAPI\n");
  setup_dmapi();

  // setup event disposition for mount
  printf("Setting disposition for MOUNT events\n");
  register_mount();

  // recover existing tokens for mount events
  printf("Recovering existing tokens for MOUNT events\n");
  token_recovery(DM_EVENT_MOUNT);

  // check if filesystem is mounted
  if(!filesystem_is_mounted()) {
    fprintf(stderr,"Filesystem %s does not appear as mounted\n",fsname);
  }

  // try to finalize inizialization
  finalize_init();

  // start main loop
  printf("Starting main event loop\n");
  event_loop();
  return(0);
}

// finalize initialization
void finalize_init() {

  // if filesystem is not mounted go ahead
  if(!filesystem_is_mounted()) return;

  // get filesystem handle
  if (dm_path_to_fshandle(fsname, &fs_hanp, &fs_hlen) == -1) {
    fprintf(stderr, "dm_path_to_fshandle, %d/%s\n", errno, strerror(errno));
    exit(1);
  }

  // setup event disposition for read, write and truncate events
  printf("Setting disposition for READ, WRITE and TRUNCATE DMAPI events\n");
  set_events(fs_hanp, fs_hlen);

  // recover existing tokens (if any)
  printf("Recovering existing tokens for MOUNT, READ, WRITE and TRUNCATE events\n");
  token_recovery(DM_EVENT_MOUNT);
  token_recovery(DM_EVENT_READ);
  token_recovery(DM_EVENT_WRITE);
  token_recovery(DM_EVENT_TRUNCATE);

  global_state=0;

  printf("Initialization finished\n");
}

// main event loop
void event_loop() {
  void *msgbuf;
  size_t bufsize, rlen;
  int error;
  dm_eventmsg_t *msg;
  pid_t process_id;

  void            *hanp;
  size_t           hlen;
  dm_data_event_t *msgev;

  int first_time=1;

  // define inizial message buffer size
  bufsize = sizeof(dm_eventmsg_t) + sizeof(dm_data_event_t) + HANDLE_LEN;
  bufsize *= 16;
  msgbuf  = (void *)malloc(bufsize);
  if (msgbuf == NULL) {
    fprintf(stderr,"Can't allocate memory for buffer\n");
    goto out;
  }

  // start infinite loop
  for (;;) {
    // cleanup all finished children
    while (child_proc_count) {
      process_id = waitpid((pid_t) -1, NULL, WNOHANG);
      if (process_id < 0) { // waitpid error
        fprintf(stderr, "cannot waitpid\n");
        exit(1);
      } else if (process_id == 0)  break; // no child to wait
      else {
        child_proc_count--;  // cleaned up one child
        if(Verbose) {
           fprintf(stderr,"Cleanup child with pid %d\n",process_id);
        }
      }
    }

    // if initialization is not finished, try to finalize
    if(global_state) finalize_init();

    // check if filesystem is mounted, otherwise exit
    if(!global_state&&!filesystem_is_mounted()) exit_program(0);

    // sleep 10 ms
    usleep(10000);

    // check if maximum number of children has been reached
    if(child_proc_count>=MAXIMUM_ALLOWED_CHILDREN) {
      if(first_time) printf("Maximum number of children reached %d/%d\n",child_proc_count,MAXIMUM_ALLOWED_CHILDREN);
      first_time=0;
      continue;
    }

    if(!first_time) {
      printf("Number of children back to normality %d/%d\n",child_proc_count,MAXIMUM_ALLOWED_CHILDREN);
      first_time=1;
    }

    // get all available event messages with unblocking call
    error = dm_get_events(sid, ALL_AVAIL_MSGS, 0, bufsize, msgbuf, &rlen);
    if (error == -1) {
      if (errno == E2BIG) {
        // message buffer was too small, enlarge it
        free(msgbuf);
        msgbuf = (void *)malloc(rlen);
        if (msgbuf == NULL) {
          fprintf(stderr,"Can't resize msg buffer\n");
          goto out;
        }
        continue;
      }
      // we got an error while getting events, or simply there were no events
      // for the moment continue, but should improve error handling here
      continue;
    }

    // examine each message and dispatch children to manage events and respond to client
    msg = (dm_eventmsg_t *)msgbuf;
    while (msg != NULL ) {
      if (Verbose) {
        fprintf(stderr, "Received %s, sid %llx token %llx %llx\n", (msg->ev_type == DM_EVENT_READ ? "READ" : 
                                                         (msg->ev_type == DM_EVENT_WRITE ? "WRITE" : (msg->ev_type == DM_EVENT_TRUNCATE ? "TRUNCATE" : "MOUNT"))), 
                                                         sid, msg->ev_token.high, msg->ev_token.low);
      }

      // get file handle 
      msgev  = DM_GET_VALUE(msg, ev_data, dm_data_event_t *);
      hanp = DM_GET_VALUE(msgev, de_handle, void *);
      hlen = DM_GET_LEN(msgev, de_handle);
     
      switch (msg->ev_type) {
         case DM_EVENT_MOUNT:
           spawn_child(msg->ev_token, hanp, hlen, "MOUNT");
           break;
         case DM_EVENT_READ:
           spawn_child(msg->ev_token, hanp, hlen, "READ");
           break;
         case DM_EVENT_WRITE:
           spawn_child(msg->ev_token, hanp, hlen, "WRITE");
           break;
         case DM_EVENT_TRUNCATE:
           spawn_child(msg->ev_token, hanp, hlen, "TRUNC");
           break;
         default:
           fprintf(stderr,"Invalid msg type %d\n", msg->ev_type);
           break;
      }
      // go to next event
      msg = DM_STEP_TO_NEXT(msg, dm_eventmsg_t *);
    }
  }

out:
  if (msgbuf != NULL) free(msgbuf);
  exit_program(0);
}

// for a child and manage the supplied event
void spawn_child(dm_token_t token, void *hanp, size_t hlen, char *action) {
  pid_t mypid;
  pid_t pid;
  char sidbuf[32];
  char tokenbufh[32];
  char tokenbufl[32];
  dm_attrname_t attrname;
  dm_attrlist_t bufp[1024];
  size_t rlenp;
  dm_stat_t statp;
  FILE* mnttab;
  struct mntent *ent;
  struct statfs sbuf;
  dm_ino_t inop;
  char s[512];
  char hname[512];
  char *yamssRecallPath="/system/YAMSS_DMRECALL";
  char *yamssDMAPIPath="/system/YAMSS_DMERROR";
  int fd;
  int i;
  int ret;
  unsigned char c;
  char extobj[1024];
  char inode[32];
  time_t rectime, now;
  struct stat filebuf;

  // fork a new child
  pid = fork();
  if (pid == 0) {
    // fork succeeded, we are in the child now
    mypid=getpid();

    // block signals
    block_signals(mypid);

    sprintf(sidbuf, "%llx", sid);
    sprintf(tokenbufh, "%llx", token.high);
    sprintf(tokenbufl, "%llx", token.low);
    if (Verbose) {
       fprintf(stderr, "%d: New child spawn with action=%s, sidbuf=%s, tokenbufh=%s, tokenbufl=%s\n",
               mypid, action, sidbuf, tokenbufh, tokenbufl);
    }

    // green light to mount requests
    if(!strcmp(action,"MOUNT")) {
      if (dm_respond_event(sid, token, DM_RESP_CONTINUE, 0, 0, NULL)) {
        fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno));
        exit(1);
      }
      if(Verbose) fprintf(stderr,"%d: MOUNT request served\n", mypid);
      exit(0);
    }

    // give error message to write and truncate events
    // i.e. we don't allow modifications of files on tape
    if(strcmp(action,"READ")) {
      // tell client that writing is not permitted
      if (dm_respond_event(sid, token, DM_RESP_ABORT, EPERM, 0, NULL)) {
        fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno)); 
        exit(1);
      }
      if(Verbose) fprintf(stderr,"%d: %s event rejected with EPERM\n", mypid, action);
      exit(0);
    }

    // get dmapi extended attribute with tsm external object id
    memset((void *)&attrname.an_chars[0], 0, DM_ATTR_NAME_SIZE);
    memcpy((void *)&attrname.an_chars[0], "IBMObj", 6);
    ret = dm_get_dmattr(sid, hanp, hlen, DM_NO_TOKEN, &attrname, sizeof(bufp), bufp, &rlenp);
    if(ret==-1) {
      fprintf(stderr,"%d: dm_get_dmattr: failed, %s\n", mypid, strerror(errno));
      // if filesystem is not mounted don't reply with error
      // it might be a normal mmshutdown and event will be taken over
      if(!filesystem_is_mounted()) exit(1);
      if (dm_respond_event(sid, token, DM_RESP_ABORT, EBUSY, 0, NULL)) {
        fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno));
      }
      exit(1);
    }

    // get file inode number
    ret = dm_handle_to_ino(hanp, hlen, &inop);
    if(ret==-1) {
      fprintf(stderr,"%d: dm_handle_to_ino: failed, %s\n", mypid, strerror(errno));
      // if filesystem is not mounted don't reply with error
      // it might be a normal mmshutdown and event will be taken over
      if(!filesystem_is_mounted()) exit(1);
      if (dm_respond_event(sid, token, DM_RESP_ABORT, EBUSY, 0, NULL)) {
        fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno));
      }
      exit(1);
    }

    // get file attributes structure
    ret=dm_get_fileattr(sid,hanp,hlen,DM_NO_TOKEN,DM_AT_STAT,&statp);
    if(ret==-1) {
      fprintf(stderr,"%d: dm_get_fileattr: failed, %s\n", mypid, strerror(errno));
      // if filesystem is not mounted don't reply with error
      // it might be a normal mmshutdown and event will be taken over
      if(!filesystem_is_mounted()) exit(1);
      if (dm_respond_event(sid, token, DM_RESP_ABORT, EBUSY, 0, NULL)) {
        fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno));
      }
      exit(1);
    }

    // determine mount point for the requested file
    mnttab = fopen("/etc/mtab", "r");
    ent=getmntent(mnttab);
    while(ent) {
      if(!statfs(ent->mnt_dir, &sbuf)) {
        if(statp.dt_dev==*(int*)&sbuf.f_fsid) {
          break;
        }
      }
      ent = getmntent(mnttab);
    }
    fclose(mnttab);
    if(!ent) {
      fprintf(stderr,"%d: cannot determine mountpoint\n", mypid);
      // if filesystem is not mounted don't reply with error
      // it might be a normal mmshutdown and event will be taken over
      if(!filesystem_is_mounted()) exit(1);
      if (dm_respond_event(sid, token, DM_RESP_ABORT, EBUSY, 0, NULL)) {
        fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno));
      }
      exit(1);
    }

    printf("%d: serving %s event for file on mount point %s with inode number %llu and external object id ", mypid, action, ent->mnt_dir, inop);
    for(i=0;i<EXTOBJID_LEN;i++) {
      c=((unsigned char*)bufp)[i];
      sprintf(extobj+i*2,"%02X",c);
    }
    *(extobj+i*2)=0;
    printf("%s\n",extobj);

    // get host name
    if(gethostname(hname,512)) {
      // this should never happen... but don't need to fail
      fprintf(stderr,"%d: cannot get hostname. Setting to dummy\n", mypid);
      sprintf(hname,"dummy");      
    }

    // generate temporary file with unique name containing the external object id of the file to be recalled
    sprintf(s,"%s%s/%s.%d.XXXXXXXX",ent->mnt_dir,yamssRecallPath,hname,mypid);
    if((fd=mkstemp(s))<0) {
      fprintf(stderr,"%d: cannot generate temporary file %s\n",mypid, s);
      // if filesystem is not mounted don't reply with error
      // it might be a normal mmshutdown and event will be taken over
      if(!filesystem_is_mounted()) exit(1);      
      if (dm_respond_event(sid, token, DM_RESP_ABORT, EBUSY, 0, NULL)) {
        fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno));
      }
      exit(1);
    }

    // write external object id, mount point and inode number into temporary file
    sprintf(inode,"%llu",inop);
    if(write(fd,extobj,strlen(extobj))<0 || write(fd," ",1)<0 || 
       write(fd,ent->mnt_dir,strlen(ent->mnt_dir))<0 || write(fd," ",1)<0 ||
       write(fd,inode,strlen(inode))<0 || write(fd,"\n",1)<0) {

      fprintf(stderr,"%d: cannot write into temporary file %s, %d/%s\n",mypid, s,errno,strerror(errno));
      close(fd);
      if(unlink(s)<0) {
        fprintf(stderr,"%d: cannot unlink temporary file %s, %d/%s\n",mypid, s, errno,strerror(errno));
      }
      // if filesystem is not mounted don't reply with error
      // it might be a normal mmshutdown and event will be taken over
      if(!filesystem_is_mounted()) exit(1);
      if (dm_respond_event(sid, token, DM_RESP_ABORT, EBUSY, 0, NULL)) {
        fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno));
      }
      exit(1);
    }
    close(fd);

    // save start time
    rectime=time(NULL);

    // Poll until file is on disk
    while(1) {
      // get file attributes structure
      ret=dm_get_fileattr(sid,hanp,hlen,DM_NO_TOKEN,DM_AT_STAT,&statp);
      if(ret==-1) {
        fprintf(stderr,"%d: dm_get_fileattr: failed, %s\n", mypid, strerror(errno));
      // if filesystem is not mounted don't reply with error
      // it might be a normal mmshutdown and event will be taken over
      if(!filesystem_is_mounted()) exit(1);
        if (dm_respond_event(sid, token, DM_RESP_ABORT, EBUSY, 0, NULL)) {
          fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno));
        }
        exit(1);
      }

      if(Verbose&&statp.dt_blocks!=0) {
        fprintf(stderr,"%d: recalling inode %s dt_blocks %llu dt_size %lld dt_blksize %u\n", mypid, inode, statp.dt_blocks, statp.dt_size, statp.dt_blksize);
      }

      // check if IBMObj is there
      memset((void *)&attrname.an_chars[0], 0, DM_ATTR_NAME_SIZE);
      memcpy((void *)&attrname.an_chars[0], "IBMObj", 6);
      ret = dm_get_dmattr(sid, hanp, hlen, DM_NO_TOKEN, &attrname, sizeof(bufp), bufp, &rlenp);
      if(ret==-1&&errno!=ENOENT) {
        // if filesystem is not mounted don't reply with error
        // it might be a normal mmshutdown and event will be taken over
        if(!filesystem_is_mounted()) exit(1);
        fprintf(stderr,"%d: dm_get_dmattr: failed, %s\n", mypid, strerror(errno));
        if (dm_respond_event(sid, token, DM_RESP_ABORT, EBUSY, 0, NULL)) {
          fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno));
        }
        exit(1);
      } else if(ret==-1) {
        if(Verbose) {
          fprintf(stderr,"%d: IBMObj has been removed for inode %s\n", mypid, inode);
        }
      }

      // check if number of blocks on disk are OK and if IBMObj has been removed
      if(statp.dt_blocks>=(statp.dt_size/statp.dt_blksize)&&ret==-1&&errno==ENOENT) {
        // File has been recalled
        if (dm_respond_event(sid, token, DM_RESP_CONTINUE, 0, 0, NULL)) {
          fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno));
          // if filesystem is not mounted don't reply with error
          // it might be a normal mmshutdown and event will be taken over
          if(!filesystem_is_mounted()) exit(1);
          if (dm_respond_event(sid, token, DM_RESP_ABORT, EBUSY, 0, NULL)) {
            fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno));
          }
          exit(1);
        }
        printf("%d: %s event for inode %llu on mount point %s served correctly\n", mypid, action, inop, ent->mnt_dir);
        exit(0);
      }
      sleep(RECALL_CHECK_SLEEP_TIME);

      // check if an error flag file has been raised
      sprintf(s,"%s%s/%s",ent->mnt_dir,yamssDMAPIPath,inode);
      if(!stat(s, &filebuf) && filebuf.st_ctime>rectime) {
        fprintf(stderr,"%d: error flag found - cannot recall inode %s on disk\n", mypid, inode);
        // respond EIO in this case to differentiate with EBUSY generic errors
        if (dm_respond_event(sid, token, DM_RESP_ABORT, EBUSY, 0, NULL)) {
          fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno));
        }
        exit(1);
      }

      // check if recall timeout expired 
      now=time(NULL);
      if((now-rectime)>RECALL_TIMEOUT) {
        fprintf(stderr,"%d: timed out - cannot recall inode %s on disk after %d seconds\n", mypid, inode, RECALL_TIMEOUT);
        if (dm_respond_event(sid, token, DM_RESP_ABORT, ETIMEDOUT, 0, NULL)) {
          fprintf(stderr, "%d: dm_respond_event failed, %d/%s\n", mypid, errno, strerror(errno));
        }
        exit(1);
      }
    }

  }
  // fork failed
  if (pid < 0) {
    fprintf(stderr,"Can't fork child\n");
    if (dm_respond_event(sid, token, DM_RESP_ABORT, EBUSY, 0, NULL)) {
      fprintf(stderr, "dm_respond_event failed, %d/%s\n", errno, strerror(errno));
    }
    return;
  }

  // only the father can reach here, increase child counter
  child_proc_count++;

  return;

}

// token recovery
void token_recovery(int event_rec) {
  int           error = 0;
  u_int         nbytes, ntokens = 0, ret_ntokens, i;
  dm_token_t    *tokenbuf = NULL, *tokenptr;
  size_t        buflen=0, ret_buflen;
  char          *msgbuf = NULL;
  dm_eventmsg_t *msg;
  void            *hanp;
  size_t           hlen;
  dm_data_event_t *msgev; 

  // Initial sizes for the token and message buffers
  ret_buflen=16*(sizeof(dm_eventmsg_t)+sizeof(dm_data_event_t)+HANDLE_LEN);
  ret_ntokens = 16;

  // get the list of tokens
  do {
    dm_token_t *tmpbuf;
    ntokens = (ntokens != ret_ntokens) ? ret_ntokens : ntokens*2;
    nbytes = ntokens * (sizeof(dm_token_t) + sizeof(dm_vardata_t));
    tmpbuf = realloc(tokenbuf, nbytes);
    if (tmpbuf == NULL) {
      fprintf(stderr,"Can't malloc %d bytes for tokenbuf\n", nbytes);
      exit_program(0);
    }

    tokenbuf = tmpbuf;
    error = dm_getall_tokens(sid, ntokens, tokenbuf, &ret_ntokens);
  } while (error && errno == E2BIG);

  if (error) {
    fprintf(stderr,"Can't get all outstanding tokens\n");
    exit_program(0);
  }

  tokenptr = tokenbuf;
  for (i = 0; i < ret_ntokens; i++) {

    do {
      char *tmpbuf;
      buflen = (buflen != ret_buflen) ? ret_buflen : buflen * 2;
      tmpbuf = realloc(msgbuf, buflen);
      if (tmpbuf == NULL) {
        fprintf(stderr,"Can't malloc %lu bytes for msgbuf\n", (long unsigned int)buflen);
        exit_program(0);
      }
      msgbuf = tmpbuf;
      error = dm_find_eventmsg(sid, *tokenptr, buflen, msgbuf, &ret_buflen);
    } while (error && errno == E2BIG);
    if (error) {
      fprintf(stderr,"Can't find the event message for token %llu %llu\n", tokenptr->high,tokenptr->low);
      exit_program(0);
    }

    msg = (dm_eventmsg_t *) msgbuf;
    while (msg != NULL) {
      // get file handle 
      msgev  = DM_GET_VALUE(msg, ev_data, dm_data_event_t *);
      hanp = DM_GET_VALUE(msgev, de_handle, void *);
      hlen = DM_GET_LEN(msgev, de_handle);

      if(event_rec==msg->ev_type) {
        if (Verbose)
          fprintf(stderr,"Recovering outstanding event for token %llu %llu\n",tokenptr->high,tokenptr->low);

        switch (msg->ev_type) {
           case DM_EVENT_MOUNT:
           spawn_child(msg->ev_token, hanp, hlen, "MOUNT");
           break;
         case DM_EVENT_READ:
           spawn_child(msg->ev_token, hanp, hlen, "READ");
           break;
         case DM_EVENT_WRITE:
           spawn_child(msg->ev_token, hanp, hlen, "WRITE");
           break;
         case DM_EVENT_TRUNCATE:
           spawn_child(msg->ev_token, hanp, hlen, "TRUNC");
           break;
         default:
           fprintf(stderr,"Invalid msg type %d\n", msg->ev_type);
           break;
        }
      }
      // go to next event
      msg = DM_STEP_TO_NEXT(msg, dm_eventmsg_t *);
    }

    tokenptr++;
  }

  if (tokenbuf) free(tokenbuf);
  if (msgbuf) free(msgbuf);

}

// Register for mount event
void register_mount() {
  dm_eventset_t eventlist;

  DMEV_ZERO(eventlist);
  DMEV_SET(DM_EVENT_MOUNT, eventlist);

  if (dm_set_disp(sid, DM_GLOBAL_HANP, DM_GLOBAL_HLEN, DM_NO_TOKEN, &eventlist, DM_EVENT_MAX) == -1) {
    fprintf(stderr, "dm_set_disp, %d/%s\n", errno, strerror(errno));
    exit(1);
  }
  printf("Registered for MOUNT\n");
}

// Set the event disposition 
void set_events() {
  dm_eventset_t eventlist;

  DMEV_ZERO(eventlist);
  DMEV_SET(DM_EVENT_READ, eventlist);
  DMEV_SET(DM_EVENT_WRITE, eventlist);
  DMEV_SET(DM_EVENT_TRUNCATE, eventlist);

  if (dm_set_disp(sid, fs_hanp, fs_hlen, DM_NO_TOKEN, &eventlist, DM_EVENT_MAX) == -1) {
    fprintf(stderr, "dm_set_disp, %d/%s\n", errno, strerror(errno));
    exit(1);
  }
  printf("Registered for READ, WRITE and TRUNCATE\n");
}


// setup signal handlers
void setup_signals() {
  struct sigaction act;

  act.sa_handler = exit_program;
  act.sa_flags   = 0;
  if(sigemptyset(&act.sa_mask)      ||
     sigaction(SIGHUP, &act, NULL)  ||
     sigaction(SIGINT, &act, NULL)  ||
     sigaction(SIGQUIT, &act, NULL) ||
     sigaction(SIGTERM, &act, NULL) ||
     sigaction(SIGUSR1, &act, NULL) ||
     sigaction(SIGUSR2, &act, NULL)) {
     fprintf(stderr,"Can't setup signals\n");
     exit(1);
  }

}

// block signals (called from child processes)
void block_signals(pid_t mypid) {
  sigset_t mask;
  if(sigemptyset (&mask)        ||
     sigaddset (&mask, SIGHUP)  ||
     sigaddset (&mask, SIGINT)  ||
     sigaddset (&mask, SIGQUIT) ||
     sigaddset (&mask, SIGTERM) ||
     sigaddset (&mask, SIGUSR1) ||
     sigaddset (&mask, SIGUSR2) ||
     sigprocmask(SIG_BLOCK, &mask, NULL)) {
     fprintf(stderr,"%d: Can't block signals\n",mypid);
     exit(1);
  }

}

void setup_dmapi() {
  char *cp;

  if (dm_init_service(&cp) == -1)  {
    fprintf(stderr,"Can't init dmapi\n");
    exit(1);
  }
  if (strcmp(cp, DM_VER_STR_CONTENTS)) {
    fprintf(stderr,"Compiled for a different version\n");
    exit(1);
  }

  find_session();
}

static int session_compare(const void *a, const void *b) {
  return(*((dm_sessid_t *)a) - *((dm_sessid_t *)b));
}

// try to assume or create session
void find_session() {
  char buffer[DM_SESSION_INFO_LEN];
  dm_sessid_t *sidbuf;
  dm_sessid_t new_session;
  u_int allocelem;
  u_int nelem;
  size_t rlen;
  int error;
  u_int i;

start_seek:
  sidbuf = NULL;
  allocelem = 0;
  // Retrieve the list of all active sessions
  nelem = 100;
  do {
    if (allocelem < nelem) {
      allocelem = nelem;
      sidbuf = realloc(sidbuf, nelem * sizeof(*sidbuf));
      if (sidbuf == NULL) {
        fprintf(stderr, "realloc of %d bytes failed\n", (int)(nelem * sizeof(*sidbuf)));
        exit(1);
      }
    }
    error = dm_getall_sessions(allocelem, sidbuf, &nelem);
  } while (error < 0 && errno == E2BIG);

  if (error < 0) {
    fprintf(stderr, "unexpected dm_getall_sessions failure, %s\n", strerror(errno));
    free(sidbuf);
    exit(1);
  }

  // check if yamss recall session exists, if yes try to assume it
  for (i = 0; i < nelem; i++) {
    error = dm_query_session(sidbuf[i], sizeof(buffer), buffer, &rlen);
    if (error < 0) {
      fprintf(stderr, "unexpected dm_query_session failure, %s\n", strerror(errno));
      free(sidbuf);
      exit(1);
    }
    if (!strncmp(buffer, DMAPI_SESSION_NAME, strlen(DMAPI_SESSION_NAME))) break;
  }
  if (i < nelem) {
    // session exists
    if(Verbose)
      fprintf(stderr, "Session %s alredy existing, try to assume it\n",DMAPI_SESSION_NAME);
    sid = (dm_sessid_t)sidbuf[i];
    if (dm_create_session(sid, DMAPI_SESSION_NAME, &new_session) != 0) {
      if(errno==EEXIST) {
        if(Verbose)
          fprintf(stderr, "Session %s cannot be assumed, sleep %d seconds and retry\n",DMAPI_SESSION_NAME,DMAPI_YAMSS_SLEEP_TIME);
        free(sidbuf);
        sleep(DMAPI_YAMSS_SLEEP_TIME);
        goto start_seek;
      }
      fprintf(stderr, "dm_create_session failed, %s\n", strerror(errno));
      free(sidbuf);
      exit(1);
    } else {
      printf("Session %s assumed\n",DMAPI_SESSION_NAME);
      free(sidbuf);
      return;
    }
  }

  // the session did not exist, create it
  if (dm_create_session(DM_NO_SESSION, DMAPI_SESSION_NAME, &new_session) != 0) {
    fprintf(stderr, "dm_create_session failed, %s\n", strerror(errno));
    free(sidbuf);
    exit(1);
  }
  printf("Session %s created\n",DMAPI_SESSION_NAME);

  // retrieve again the list of sessions to check if there are multiple ones
  // managing the case where two or more nodes by chance created a new
  // session at the same time 
  do {
    if (allocelem < nelem) {
      allocelem = nelem;
      sidbuf = realloc(sidbuf, nelem * sizeof(*sidbuf));
      if (sidbuf == NULL) {
        fprintf(stderr, "realloc of %d bytes failed\n", (int)(nelem * sizeof(*sidbuf)));
        exit(1);
      }
    }
    error = dm_getall_sessions(allocelem, sidbuf, &nelem);
  } while (error < 0 && errno == E2BIG);

  if (error < 0) {
    fprintf(stderr, "dm_getall_sessions failed, %s\n", strerror(errno));
    free(sidbuf);
    exit(1);
  }

  // sort sessions looking for the lowest id
  qsort(sidbuf, nelem, sizeof(sidbuf[0]), session_compare);
  for (i = 0; i < nelem; i++) {
    error = dm_query_session(sidbuf[i], sizeof(buffer), buffer, &rlen);
    if (error < 0) {
      fprintf(stderr, "dm_query_session failed, %s\n", strerror(errno));
      free(sidbuf);
      exit(1);
    }
    if (!strncmp(buffer, DMAPI_SESSION_NAME, strlen(DMAPI_SESSION_NAME))) break;
  }
  if (i == nelem) {
    fprintf(stderr, "can't find the session we created\n");
    free(sidbuf);
    exit(1);
  }
  sid = (dm_sessid_t)sidbuf[i];
  free(sidbuf);

  // if the session is not our one, destroy it
  if (sid != new_session) {
    printf("Duplicate %s session found, destroying our one, sleep %d seconds and retry\n",DMAPI_SESSION_NAME,DMAPI_YAMSS_SLEEP_TIME);
    if (dm_destroy_session(new_session)) {
      fprintf(stderr, "dm_destroy_session failed, %s\n", strerror(errno));
      exit(1);
    }
    sleep(DMAPI_YAMSS_SLEEP_TIME);
    goto start_seek; 
  }
}


int filesystem_is_mounted() {
  FILE* mnttab;
  struct mntent *ent;

  // determine mount point for the requested file
  mnttab = fopen("/etc/mtab", "r");
  ent=getmntent(mnttab);
  while(ent) {
    if(!strcmp(ent->mnt_dir, fsname)) break;
    ent = getmntent(mnttab);
  }
  fclose(mnttab);
  if(!global_state&&!ent) {
    fprintf(stderr,"Filesystem %s does not appear as mounted\n",fsname);
  }

  if(!ent) return 0;

  if(global_state) printf("Filesystem %s is mounted\n",fsname);

  return 1;
}

void exit_program(int x) {
  int error;

  // graceful exit
  if(x==0)
    printf("Error detected: waiting for all children to die... this may take a lot\nUse SIGKILL if you know what you are doing and want to terminate immediately\n");
  else 
    printf("Caught signal %d: waiting for all children to die... this may take a lot\nUse SIGKILL if you know what you are doing and want to terminate immediately\n", x);

  while (wait(NULL) > 0);
  printf("No children are running\n");

  if(x==0) {
    printf("The DMAPI session will not be destroyed\n");
  } else if(x==SIGTERM) {
     printf("Having caught SIGTERM the DMAPI session will not be destroyed\n");
  } else {
    printf("As caught signal is not SIGTERM the DMAPI session will be destroyed\nHope you know what you are doing\n");
    error = dm_destroy_session(sid);
    if (error == -1) {
      fprintf(stderr, "dm_destroy_session, %d/%s\n", errno, strerror(errno));
    }
  }

  fprintf(stdout, "Exiting main program\n");

  exit(0);
}


