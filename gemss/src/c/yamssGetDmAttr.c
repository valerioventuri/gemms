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

   int i;

   dm_attrlist_t bufp[1024];

   void *dmhandle=NULL;
   size_t dmhandle_len=0;
   int ret;
   dm_sessid_t sid = DM_NO_SESSION;
   dm_attrname_t attrname;
   size_t rlen;


   if(argc<3) {
      fprintf(stderr,"Usage: %s <file name> <attribute name>\n", argv[0]);
      return 1;
   }

   if(strlen(argv[2])>DM_ATTR_NAME_SIZE-1) {
      fprintf(stderr,"Usage: %s <file name> <attribute name> <attribute value>\n", argv[0]); 
      fprintf(stderr,"<attribute name> must be %d characters long at maximum\n", DM_ATTR_NAME_SIZE-1);
      return 1;
   }

   if (dm_create_session(DM_NO_SESSION, "dmattr", &sid)!=0) {
      fprintf(stderr,"dm_create_session: failed, %s\n", strerror(errno));
      return 1;
   }

   if (dm_path_to_handle(argv[1], &dmhandle, &dmhandle_len) != 0) {
      fprintf(stderr,"dm_path_to_handle: failed, %s\n", strerror(errno));
      dm_destroy_session(sid);      
      return 1;
   }

   memset((void *)&attrname.an_chars[0], 0, DM_ATTR_NAME_SIZE);
   memcpy((void *)&attrname.an_chars[0], argv[2], strlen(argv[2]));
   ret = dm_get_dmattr(sid, dmhandle, dmhandle_len,
                            DM_NO_TOKEN, &attrname, sizeof(bufp), bufp, &rlen);
   if(ret==-1) {
      fprintf(stderr,"dm_get_dmattr: failed, %s\n", strerror(errno));
      dm_handle_free(dmhandle, dmhandle_len);
      dm_destroy_session(sid);
      return 1;
   }

   for(i=0;i<rlen;i++) {
      unsigned char c=((unsigned char*)bufp)[i];
      printf("%02x",c);
   }
   printf("\n");

   dm_handle_free(dmhandle, dmhandle_len);
   dm_destroy_session(sid);
   return 0;
}

