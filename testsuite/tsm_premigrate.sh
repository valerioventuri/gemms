#!/bin/bash

echo CREAZIONE DEI FILE DA preMIGRARE

for i in `seq 9`; do dd if=/dev/urandom of=/gpfs/gemss_test/test/data1/greg$i bs=2048 count=1000; done

for i in `seq 9`; do dd if=/dev/urandom of=/gpfs/gemss_test/test/data2/greg$i bs=2048 count=1000; done

for i in `seq 9`; do dd if=/dev/urandom of=/gpfs/gemss_test2/test/data3/greg$i bs=2048 count=1000; done

for i in `seq 9`; do dd if=/dev/urandom of=/gpfs/gemss_test2/test/data4/greg$i bs=2048 count=1000; done

echo ATTESA DI 5 minuti CHE IL FILE INVECCHI poi si migrano;
sleep 300

ssh tsm-hsm-13 "dsmmigrate -p /gpfs/gemss_test/test/data1/greg*"

ssh tsm-hsm-12 "dsmmigrate -p /gpfs/gemss_test/test/data2/greg*"

ssh tsm-hsm-13 "dsmmigrate -p /gpfs/gemss_test2/test/data3/greg*"

ssh tsm-hsm-12 "dsmmigrate -p /gpfs/gemss_test2/test/data4/greg*"

echo CONTROLLARE I LOG IN /gpfs/gemss_test/system/YAMSS_LOG/startpolicy.log
echo E IN /gpfs/gemss_test2/system/YAMSS_LOG/startpolicy.log
