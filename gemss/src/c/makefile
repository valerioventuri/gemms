MYLFOPTS=-D_LARGE_FILE_SOURCE -D_FILE_OFFSET_BITS=64

MYCCOPTS=-O2 -Wall

all: yamssPreloadStat.so yamssSigTermUnblock yamssRm yamssStubbify yamssRemoveWrapper yamssRecall yamssDirectoryNotify yamssGetDmAttr yamssSetDmAttr yamssRemoveDmAttr yamssAbortDmSession yamssRecallDaemon yamssShowPoolOccupancy

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

yamssRemoveDmAttr: yamssRemoveDmAttr.c
	gcc ${MYCCOPTS} yamssRemoveDmAttr.c -ldmapi -o yamssRemoveDmAttr

yamssSetDmAttr: yamssSetDmAttr.c
	gcc ${MYCCOPTS} yamssSetDmAttr.c -ldmapi -o yamssSetDmAttr

yamssAbortDmSession: yamssAbortDmSession.c
	gcc ${MYCCOPTS} yamssAbortDmSession.c -ldmapi -o yamssAbortDmSession

yamssRecallDaemon: yamssRecallDaemon.c
	gcc ${MYCCOPTS} -ldmapi yamssRecallDaemon.c -o yamssRecallDaemon -lgpfs

yamssShowPoolOccupancy: yamssShowPoolOccupancy.c
	gcc ${MYCCOPTS} -ldmapi yamssShowPoolOccupancy.c -oyamssShowPoolOccupancy -lgpfs

clean:
	rm -f yamssPreloadStat.so yamssRm yamssStubbify yamssRemoveDmAttr yamssRemoveWrapper yamssRecall yamssSigTermUnblock yamssDirectoryNotify yamssGetDmAttr yamssSetDmAttr yamssAbortDmSession yamssRecallDaemon yamssShowPoolOccupancy

install:
	mkdir -p /usr/local/yamss/bin	
	mkdir -p /usr/local/yamss/lib
	cp -a yamssRecall yamssRm yamssStubbify yamssRemoveWrapper yamssRemoveDmAttr yamssSigTermUnblock yamssDirectoryNotify yamssGetDmAttr yamssSetDmAttr yamssAbortDmSession yamssRecallDaemon yamssShowPoolOccupancy /usr/local/yamss/bin
	cp -a yamssPreloadStat.so /usr/local/yamss/lib
