#!/bin/sh
#variables
SERVER_EXE=TFTPserver
CLIENT_EXE=TFTPclient

SERVER_FOLDER_NAME=ficherosTFTPserver
CLIENT_FOLDER_NAME=ficherosTFTPclient

FILE1=test1.gif
FILE2=test2.gif
FILE3=test3.gif
FILE4=test4.gif
FILE5=test5.gif
FILE6=test6.gif

#remove compilation
if [ -f *.o ]; then
	rm *.o
fi
if [ -f TFTPserver ]; then
	rm TFTPserver
fi
if [ -f TFTPclient ]; then
	rm TFTPclient
fi
#remove last execution results
if [ -f [0-9]*.txt ]; then
	rm [0-9]*.txt
fi
if [ -f peticiones.log ]; then
	rm peticiones.log
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
