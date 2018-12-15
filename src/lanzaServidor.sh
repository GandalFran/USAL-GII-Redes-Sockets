#!/bin/sh
#vars
SERVER_IP=olivo

FILE1=fichero1.txt
FILE2=fichero2.txt
FILE3=fichero3.txt
FILE4=fichero4.txt
FILE5=fichero5.txt
FILE6=fichero6.txt
#compile
make
#run
servidor
cliente $SERVER_IP TCP e $FILE1 &
cliente $SERVER_IP TCP l $FILE2 &
cliente $SERVER_IP TCP e $FILE3 &
cliente $SERVER_IP UDP e $FILE4 &
cliente $SERVER_IP UDP l $FILE5 &
cliente $SERVER_IP UDP e $FILE6 &