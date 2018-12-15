#!/bin/sh
#vars
SERVER_FOLDER_NAME=ficherosTFTPserver
CLIENT_FOLDER_NAME=ficherosTFTPclient
SERVER_IP=olivo.fis.usal.es
FILE1=test1.gif
FILE2=test2.gif
FILE3=test3.gif
FILE4=test4.gif
FILE5=test5.gif
FILE6=test6.gif
#compile
make
#run
./TFTPserver
./TFTPclient $SERVER_IP TCP e $FILE1 &
./TFTPclient $SERVER_IP TCP e $FILE2 &
./TFTPclient $SERVER_IP TCP l $FILE3 &
./TFTPclient $SERVER_IP UDP l $FILE4 &
./TFTPclient $SERVER_IP UDP e $FILE5 &
./TFTPclient $SERVER_IP UDP l $FILE6 &
