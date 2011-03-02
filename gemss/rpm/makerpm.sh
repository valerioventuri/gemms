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


. ../version 

# prepare build yamss hsm package
cat yamss.spec.template.hsm > /tmp/yamss.spec.hsm
ls /etc/profile.d/yamss.* /usr/local/yamss/bin/* /usr/local/yamss/lib/* /etc/init.d/yamssmonitor >> /tmp/yamss.spec.hsm
sed -e "s/^Version:.*/Version: $version/g" -e "s/^Release:.*/Release: $release/g"  /tmp/yamss.spec.hsm > yamss.spec.hsm

# prepare build yamss client package
cat yamss.spec.template.client > /tmp/yamss.spec.client
ls /etc/profile.d/yamss.* /usr/local/yamss/bin/yamssLs /usr/local/yamss/bin/yamssRm /usr/local/yamss/bin/yamssStubbify /usr/local/yamss/bin/yamssRecall /usr/local/yamss/bin/yamssGetStatus /usr/local/yamss/bin/yamssCommands >> /tmp/yamss.spec.client
sed -e "s/^Version:.*/Version: $version/g" -e "s/^Release:.*/Release: $release/g" /tmp/yamss.spec.client > yamss.spec.client

# build all
rpmbuild -bb yamss.spec.hsm
rpmbuild -bb yamss.spec.client

