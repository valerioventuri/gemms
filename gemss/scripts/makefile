install:
	mkdir -p /usr/local/yamss/bin
	cp -a yamss* /usr/local/yamss/bin
	rm -f /usr/local/yamss/bin/yamssmonitor
	rm -f /usr/local/yamss/bin/yamss.*
	cp -a hsmCommands hsmControl startpolicy /var/mmfs/etc
	cp -a yamssmonitor /etc/init.d
	cp -a yamss.sh yamss.csh /etc/profile.d

rpm:
	./makerpm.sh

