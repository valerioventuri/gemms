#!/bin/bash

echo CREAZIONE DEI FILE DA preMIGRARE

BASEPATH1=$(cat testsuite.conf|egrep -v '^#'|egrep '^' BASEPATH1|sed -e 's/^.*=//')

BASEPATH2=$(cat testsuite.conf|egrep -v '^#'|egrep '^' BASEPATH2|sed -e 's/^.*=//')

NFILE=$(cat testsuite.conf|egrep -v '^#'|egrep '^' NFILE|sed -e 's/^.*=//')

OFFSET=$(cat testsuite.conf|egrep -v '^#'|egrep '^' OFFSET|sed -e 's/^.*=//')

for i in `seq $NFILE` 
do 

    cnt=`echo $(($RANDOM %1000 + $OFFSET))` 
    dd if=/dev/urandom of=${BASEPATH1}/data1/greg$i bs=2048 count=$cnt 
    cnt=`echo $(($RANDOM %1000 + $OFFSET))`
    dd if=/dev/urandom of=${BASEPATH1}/data2/greg$i bs=2048 count=$cnt 
    cnt=`echo $(($RANDOM %1000 + $OFFSET))`
    dd if=/dev/urandom of=${BASEPATH2}/data3/greg$i bs=2048 count=$cnt 
    cnt=`echo $(($RANDOM %1000 + $OFFSET))`
    dd if=/dev/urandom of=${BASEPATH2}/data4/greg$i bs=2048 count=$cnt 
done

echo ATTESA DI 5 minuti CHE IL FILE INVECCHI poi si migrano;
sleep 300
date
echo "LANCIO dsmmigrate"
ssh tsm-hsm-13 "dsmmigrate -p /gpfs/gemss_test/test/data1/greg*"

ssh tsm-hsm-12 "dsmmigrate -p /gpfs/gemss_test/test/data2/greg*"

ssh tsm-hsm-13 "dsmmigrate -p /gpfs/gemss_test2/test/data3/greg*"

ssh tsm-hsm-12 "dsmmigrate -p /gpfs/gemss_test2/test/data4/greg*"

date
echo CONTROLLARE I LOG IN /gpfs/gemss_test/system/YAMSS_LOG/startpolicy.log
echo E IN /gpfs/gemss_test2/system/YAMSS_LOG/startpolicy.log
