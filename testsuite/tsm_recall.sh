#!/bin/bash

echo CREA LISTE DEI DEI FILE MIGRATI IN PRECEDENZA PER FARE RECALL ORDINATE
echo OCCORRE ESSERE SU TSM-HSM-13 (owner node)

BASEPATH1=$(cat testsuite.conf|egrep -v '^#'|egrep '^' BASEPATH1|sed -e 's/^.*=//')

BASEPATH2=$(cat testsuite.conf|egrep -v '^#'|egrep '^' BASEPATH2|sed -e 's/^.*=//')

PATHLIST1=$(cat testsuite.conf|egrep -v '^#'|egrep '^' PATHLIST1|sed -e 's/^.*=//')

PATHLIST2=$(cat testsuite.conf|egrep -v '^#'|egrep '^' PATHLIST2|sed -e 's/^.*=//')

NFILE=$(cat testsuite.conf|egrep -v '^#'|egrep '^' NFILE|sed -e 's/^.*=//')

NMEZZI= $(($NFILE/2));

mkdir $PATHLIST1
rm -f $PATHLIST1/*
touch $PATHLIST1/lista.tmp

mkdir $PATHLIST2
rm -f $PATHLIST2/*
touch $PATHLIST2/lista.tmp

for i in `seq $NMEZZI` 
do 
    echo ${BASEPATH1}/data1/greg$i>> ${PATHLIST1}/lista.tmp; 
    echo ${BASEPATH1}/data2/greg$i>> ${PATHLIST1}/lista.tmp;
    echo ${BASEPATH2}/data3/greg$i>> ${PATHLIST1}/lista.tmp; 
    echo ${BASEPATH2}/data4/greg$i>> ${PATHLIST1}/lista.tmp;
done

cd ${PATHLIST1}
dsmrecall -t -p -fi=${PATHLIST1}/lista.tmp

for i in `seq $(($NMEZZI+1)) NFILE` 
do 
    echo ${BASEPATH1}/data1/greg$i>> ${PATHLIST2}/lista.tmp; 
    echo ${BASEPATH1}/data2/greg$i>> ${PATHLIST2}/lista.tmp;
    echo ${BASEPATH2}/data3/greg$i>> ${PATHLIST2}/lista.tmp;  
    echo ${BASEPATH2}/data4/greg$i>> ${PATHLIST2}/lista.tmp;   
done

cd ${PATHLIST2}
dsmrecall -t -p -fi=${PATHLIST2}/lista.tmp
scp ${PATHLIST2}/* tsm-hsm-12:/tmp/

echo CREATE LISTE ORDINATE
sleep 5

echo PARTONO LE RECALL da TSM-HSM-13
sleep 5

cd ${PATHLIST1}
dsmrecall -t -fi=${PATHLIST1}/filelist.ordered.collection

echo PARTONO LE RECALL da TSM-HSM-12
sleep 5

ssh tsm-hsm-12 "dsmrecall -t -fi=/tmp/filelist.ordered.collection"
