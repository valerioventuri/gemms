#!/bin/bash

echo CREA LISTE DEI DEI FILE MIGRATI IN PRECEDENZA PER FARE RECALL ORDINATE
echo OCCORRE ESSERE SU TSM-HSM-13 (owner node)

mkdir /tmp/greg
rm -f /tmp/greg/*
touch /tmp/greg/lista.tmp

mkdir /tmp/greg2
rm -f /tmp/greg2/*
touch /tmp/greg2/lista.tmp

for i in `seq 4`; do echo /gpfs/gemss_test/test/data1/greg$i>> /tmp/greg/lista.tmp; 
echo /gpfs/gemss_test/test/data2/greg$i>> /tmp/greg/lista.tmp;
echo /gpfs/gemss_test2/test/data3/greg$i>> /tmp/greg/lista.tmp; 
echo /gpfs/gemss_test2/test/data4/greg$i>> /tmp/greg/lista.tmp;
done

cd /tmp/greg/ 
dsmrecall -t -p -fi=/tmp/greg/lista.tmp

for i in `seq 5 9`; do echo /gpfs/gemss_test/test/data1/greg$i>> /tmp/greg2/lista.tmp; 
echo /gpfs/gemss_test/test/data2/greg$i>> /tmp/greg2/lista.tmp;
echo /gpfs/gemss_test2/test/data3/greg$i>> /tmp/greg2/lista.tmp; 
echo /gpfs/gemss_test2/test/data4/greg$i>> /tmp/greg2/lista.tmp;
done

cd /tmp/greg2/ 
dsmrecall -t -p -fi=/tmp/greg2/lista.tmp
scp /tmp/greg2/* tsm-hsm-12:/tmp/

echo CREATE LISTE ORDINATE
sleep 5
echo PARTONO LE RECALL da TSM-HSM-13
sleep 5

cd /tmp/greg/ 
dsmrecall -t -fi=/tmp/greg/filelist.ordered.collection

echo PARTONO LE RECALL da TSM-HSM-12
sleep 5

ssh tsm-hsm-12 "dsmrecall -t -fi=/tmp/filelist.ordered.collection"
