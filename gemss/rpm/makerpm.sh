#!/bin/bash

MYPRBIN=bin
MYPRLIB=lib
MYPR=32

pkg=$(basename $(readlink -f ../) | cut -d"-" -f1)
version=$(basename $(readlink -f ../) | cut -d"-" -f2)

# prepare build yamss hsm package
cat yamss.spec.template.hsm > /tmp/yamss.spec.hsm
ls /etc/profile.d/yamss.* /usr/local/yamss/bin/* /usr/local/yamss/lib/* /etc/init.d/yamssmonitor /var/mmfs/etc/hsmControl /var/mmfs/etc/hsmCommands /var/mmfs/etc/startpolicy >> /tmp/yamss.spec.hsm
sed -e "s/^Version:.*/Version: $version/g" /tmp/yamss.spec.hsm > yamss.spec.hsm

# prepare build yamss server package
cat yamss.spec.template.server > /tmp/yamss.spec.server
ls /usr/local/yamss/bin/yamssMigrateStat /usr/local/yamss/bin/yamssRecallStat /usr/local/yamss/bin/yamssLogger /var/mmfs/etc/hsmCommands /var/mmfs/etc/startpolicy >> /tmp/yamss.spec.server
sed -e "s/^Version:.*/Version: $version/g" /tmp/yamss.spec.server > yamss.spec.server

# prepare build yamss client package
cat yamss.spec.template.client > /tmp/yamss.spec.client
ls /etc/profile.d/yamss.* /usr/local/yamss/bin/yamssLs /usr/local/yamss/bin/yamssRm /usr/local/yamss/bin/yamssStubbify /usr/local/yamss/bin/yamssRecall /var/mmfs/etc/hsmCommands >> /tmp/yamss.spec.client
sed -e "s/^Version:.*/Version: $version/g" /tmp/yamss.spec.client > yamss.spec.client

# prepare build yamss preload package
cat yamss.spec.template.preload > /tmp/yamss.spec.preload
ls /usr/local/yamss/preload/$MYPRBIN/yamssRecallWrapper /usr/local/yamss/preload/$MYPRLIB/yamssPreloadOpen.so >> /tmp/yamss.spec.preload
sed -e "s/^Version:.*/Version: $version/g" -e "s/Name:.*/Name: gemss-preload$MYPR/g" /tmp/yamss.spec.preload > yamss.spec.preload

cd ../..

cp -a $pkg-$version $pkg-server-$version
tar zcvf /usr/src/redhat/SOURCES/$pkg-server-$version.tar.gz $pkg-server-$version
rm -rf $pkg-server-$version

cp -a $pkg-$version $pkg-client-$version
tar zcvf /usr/src/redhat/SOURCES/$pkg-client-$version.tar.gz $pkg-client-$version
rm -rf $pkg-client-$version

cp -a $pkg-$version $pkg-preload$MYPR-$version
tar zcvf /usr/src/redhat/SOURCES/$pkg-preload$MYPR-$version.tar.gz $pkg-preload$MYPR-$version
rm -rf $pkg-preload$MYPR-$version

cp -a $pkg-$version $pkg-hsm-$version
tar zcvf /usr/src/redhat/SOURCES/$pkg-hsm-$version.tar.gz $pkg-hsm-$version
rm -rf $pkg-hsm-$version

cd -

# build all
rpmbuild -ba yamss.spec.hsm
rpmbuild -ba yamss.spec.server
rpmbuild -ba yamss.spec.client
rpmbuild -ba yamss.spec.preload

