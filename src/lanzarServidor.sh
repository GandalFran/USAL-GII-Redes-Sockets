#!/bin/sh
#vars
LOG_FOLDER_NAME=log
SERVER_FOLDER_NAME=ficherosTFTPserver
CLIENT_FOLDER_NAME=ficherosTFTPclient
SERVER_IP=olivo.fis.usal.es
FILE1=test1.gif
FILE2=test2.gif
FILE3=test3.gif
FILE4=test4.gif
FILE5=test5.gif
FILE6=test6.gif
#if doesn't exists folders crete, and if exists clean 
if [ ! -d "$LOG_FOLDER_NAME" ]; then
	mkdir $LOG_FOLDER_NAME
else
	rm $LOG_FOLDER_NAME/*.*
fi
if [ ! -d "$SERVER_FOLDER_NAME" ]; then
	mkdir $SERVER_FOLDER_NAME
fi
if [ ! -d "$CLIENT_FOLDER_NAME" ]; then
	mkdir $CLIENT_FOLDER_NAME
fi
#delete transferred files
if [ -f "$SERVER_FOLDER_NAME/$FILE1" ]; then
	rm $SERVER_FOLDER_NAME/$FILE1
fi
if [ -f "$SERVER_FOLDER_NAME/$FILE2" ]; then
	rm $SERVER_FOLDER_NAME/$FILE2
fi
if [ -f "$SERVER_FOLDER_NAME/$FILE5" ]; then
	rm $SERVER_FOLDER_NAME/$FILE5
fi
if [ -f "$CLIENT_FOLDER_NAME/$FILE3" ]; then
	rm $CLIENT_FOLDER_NAME/$FILE3
fi
if [ -f "$CLIENT_FOLDER_NAME/$FILE4" ]; then
	rm $CLIENT_FOLDER_NAME/$FILE4
fi
if [ -f "$CLIENT_FOLDER_NAME/$FILE6" ]; then
	rm $CLIENT_FOLDER_NAME/$FILE6
fi
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
