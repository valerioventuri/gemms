compile:
	make -C src/c
	make -C src/java
	make -C src/nailgun-0.7.1

install:
	make -C src/c install
	make -C src/java install
	make -C scripts install
	make -C lib install
	mkdir -p /usr/local/yamss/bin
	mkdir -p /usr/local/yamss/lib
	cp -a src/nailgun-0.7.1/nailgun-0.7.1.jar /usr/local/yamss/lib
	cp -a src/nailgun-0.7.1/ng /usr/local/yamss/bin

clean:
	make -C src/c clean
	make -C src/java clean
	make -C src/nailgun-0.7.1 clean

build:
	make -C rpm
