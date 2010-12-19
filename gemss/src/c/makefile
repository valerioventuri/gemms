MYLFOPTS=-D_LARGE_FILE_SOURCE -D_FILE_OFFSET_BITS=64

MYCCOPTS=-O2 -Wall

all: yamssPreloadStat.so yamssSigTermUnblock yamssRm yamssStubbify yamssRemoveWrapper yamssRecall yamssDirectoryNotify yamssGetDmAttr yamssSetDmAttr yamssRecallDaemon

yamssPreloadStat.so: yamssPreloadStat.c
	gcc -m32 -fPIC ${MYCCOPTS} --shared -Wl,-soname,yamssPreloadStat.so -ldl yamssPreloadStat.c -o yamssPreloadStat.so

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

yamssRecallDaemon: yamssRecallDaemon.c
	gcc ${MYCCOPTS} -ldmapi yamssRecallDaemon.c -o yamssRecallDaemon -lgpfs

clean:
	rm -f yamssPreloadStat.so yamssRm yamssStubbify yamssRemoveWrapper yamssRecall yamssSigTermUnblock yamssDirectoryNotify yamssGetDmAttr yamssSetDmAttr yamssRecallDaemon

install:
	mkdir -p /usr/local/yamss/bin	
	mkdir -p /usr/local/yamss/lib
	cp -a yamssRecall yamssRm yamssStubbify yamssRemoveWrapper yamssSigTermUnblock yamssDirectoryNotify yamssGetDmAttr yamssSetDmAttr yamssRecallDaemon /usr/local/yamss/bin
	cp -a yamssPreloadStat.so /usr/local/yamss/lib
