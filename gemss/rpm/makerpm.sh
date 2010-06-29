#!/bin/bash
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

MYPRBIN=bin
MYPRLIB=lib
MYPR=32

if [ $(uname -m) = "x86_64" ]; then
  MYPRBIN=bin64
  MYPRLIB=lib64
  MYPR=64
fi


. ../version 

# prepare build yamss hsm package
cat yamss.spec.template.hsm > /tmp/yamss.spec.hsm
ls /etc/profile.d/yamss.* /usr/local/yamss/bin/* /usr/local/yamss/lib/* /etc/init.d/yamssmonitor /var/mmfs/etc/hsmControl /var/mmfs/etc/hsmCommands /var/mmfs/etc/startpolicy | egrep -v "/usr/local/yamss/bin/yamssMigrateStat$|/usr/local/yamss/bin/yamssRecallStat$|/usr/local/yamss/bin/yamssLogger$|/var/mmfs/etc/hsmCommands$|/var/mmfs/etc/startpolicy$" >> /tmp/yamss.spec.hsm
sed -e "s/^Version:.*/Version: $version/g" -e "s/^Release:.*/Release: $release/g"  /tmp/yamss.spec.hsm > yamss.spec.hsm

# prepare build yamss server package
cat yamss.spec.template.server > /tmp/yamss.spec.server
ls /usr/local/yamss/bin/yamssMigrateStat /usr/local/yamss/bin/yamssRecallStat /usr/local/yamss/bin/yamssLogger /var/mmfs/etc/hsmCommands /var/mmfs/etc/startpolicy >> /tmp/yamss.spec.server
sed -e "s/^Version:.*/Version: $version/g" -e "s/^Release:.*/Release: $release/g"  /tmp/yamss.spec.server > yamss.spec.server

# prepare build yamss client package
cat yamss.spec.template.client > /tmp/yamss.spec.client
ls /etc/profile.d/yamss.* /usr/local/yamss/bin/yamssLs /usr/local/yamss/bin/yamssRm /usr/local/yamss/bin/yamssStubbify /usr/local/yamss/bin/yamssRecall /var/mmfs/etc/hsmCommands >> /tmp/yamss.spec.client
sed -e "s/^Version:.*/Version: $version/g" -e "s/^Release:.*/Release: $release/g" /tmp/yamss.spec.client > yamss.spec.client

# prepare build yamss preload package
cat yamss.spec.template.preload > /tmp/yamss.spec.preload
ls /usr/local/yamss/preload/$MYPRBIN/yamssRecallWrapper /usr/local/yamss/preload/$MYPRLIB/yamssPreloadOpen.so >> /tmp/yamss.spec.preload
sed -e "s/^Version:.*/Version: $version/g" -e "s/^Release:.*/Release: $release/g" -e "s/Name:.*/Name: gemss-preload$MYPR/g" /tmp/yamss.spec.preload > yamss.spec.preload

# build all
rpmbuild -bb yamss.spec.hsm
rpmbuild -bb yamss.spec.server
rpmbuild -bb yamss.spec.client
rpmbuild -bb yamss.spec.preload

