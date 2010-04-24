MYLFOPTS=-D_LARGE_FILE_SOURCE -D_FILE_OFFSET_BITS=64
MYPRBIN=bin
MYPRLIB=lib

ifeq ($(shell uname -m), x86_64)
  MYLFOPTS=
  MYPRBIN=bin64
  MYPRLIB=lib64
endif

MYCCOPTS=-O2 -Wall

all: yamssPreloadStat.so yamssPreloadOpen.so yamssRecallWrapper yamssSigTermUnblock yamssRm yamssStubbify yamssRemoveWrapper yamssRecall yamssDirectoryNotify yamssGetDmAttr yamssSetDmAttr

yamssPreloadOpen.so: yamssPreloadOpen.c
	gcc -fPIC ${MYCCOPTS} ${MYLFOPTS} --shared -Wl,-soname,yamssPreloadOpen.so -ldl yamssPreloadOpen.c -o yamssPreloadOpen.so

yamssPreloadStat.so: yamssPreloadStat.c
	gcc -fPIC ${MYCCOPTS} --shared -Wl,-soname,yamssPreloadStat.so -ldl yamssPreloadStat.c -o yamssPreloadStat.so

yamssRecallWrapper: yamssRecallWrapper.c
	gcc ${MYCCOPTS} yamssRecallWrapper.c -o yamssRecallWrapper
	chown nobody:nobody yamssRecallWrapper
	chmod +s yamssRecallWrapper

yamssRecall: yamssRecall.c
	gcc ${MYCCOPTS} yamssRecall.c -o yamssRecall
	chown nobody:nobody yamssRecall
	chmod +s yamssRecall

yamssRm: yamssRm.c
	gcc ${MYCCOPTS} yamssRm.c -o yamssRm
	chown nobody:nobody yamssRm
	chmod +s yamssRm

yamssStubbify: yamssStubbify.c
	gcc ${MYCCOPTS} -lgpfs yamssStubbify.c -o yamssStubbify
	chown nobody:nobody yamssStubbify
	chmod +s yamssStubbify

yamssRemoveWrapper: yamssRemoveWrapper.c
	gcc ${MYCCOPTS} yamssRemoveWrapper.c -o yamssRemoveWrapper 

yamssSigTermUnblock: yamssSigTermUnblock.c
	gcc ${MYCCOPTS} -lpthread yamssSigTermUnblock.c -o yamssSigTermUnblock

yamssDirectoryNotify: yamssDirectoryNotify.c
	gcc ${MYCCOPTS} yamssDirectoryNotify.c -o yamssDirectoryNotify

yamssGetDmAttr: yamssGetDmAttr.c
	gcc ${MYCCOPTS} yamssGetDmAttr.c -ldmapi -o yamssGetDmAttr

yamssSetDmAttr: yamssSetDmAttr.c
	gcc ${MYCCOPTS} yamssSetDmAttr.c -ldmapi -o yamssSetDmAttr


clean:
	rm -f yamssPreloadStat.so yamssPreloadOpen.so yamssRecallWrapper yamssRm yamssStubbify yamssRemoveWrapper yamssRecall yamssSigTermUnblock yamssDirectoryNotify yamssGetDmAttr yamssSetDmAttr

install:
	mkdir -p /usr/local/yamss/bin
	mkdir -p /usr/local/yamss/lib
	mkdir -p /usr/local/yamss/preload/${MYPRBIN}
	mkdir -p /usr/local/yamss/preload/${MYPRLIB}
	cp -a yamssRecallWrapper /usr/local/yamss/preload/${MYPRBIN}
	cp -a yamssRecall yamssRm yamssStubbify yamssRemoveWrapper yamssSigTermUnblock yamssDirectoryNotify yamssGetDmAttr yamssSetDmAttr /usr/local/yamss/bin
	cp -a yamssPreloadOpen.so /usr/local/yamss/preload/${MYPRLIB}
	cp -a yamssPreloadStat.so /usr/local/yamss/lib

