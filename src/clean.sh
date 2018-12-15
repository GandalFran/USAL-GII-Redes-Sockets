#!/bin/sh
#variables
SERVER_EXE=servidor
CLIENT_EXE=cliente

SERVER_FOLDER_NAME=ficherosTFTPserver
CLIENT_FOLDER_NAME=ficherosTFTPclient

FILE1=fichero1.txt
FILE2=fichero2.txt
FILE3=fichero3.txt
FILE4=fichero4.txt
FILE5=fichero5.txt
FILE6=fichero6.txt

#remove compilation
if [ -f *.o ]; then
	rm *.o
fi
if [ -f $CLIENT_EXE ]; then
	rm $CLIENT_EXE
fi
if [ -f $SERVER_EXE ]; then
	rm $SERVER_EXE
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
if [ -f "$CLIENT_FOLDER_NAME/$FILE2" ]; then
	rm $CLIENT_FOLDER_NAME/$FILE2
fi
if [ -f "$SERVER_FOLDER_NAME/$FILE3" ]; then
	rm $SERVER_FOLDER_NAME/$FILE3
fi
if [ -f "$SERVER_FOLDER_NAME/$FILE4" ]; then
	rm $SERVER_FOLDER_NAME/$FILE4
fi
if [ -f "$CLIENT_FOLDER_NAME/$FILE5" ]; then
	rm $CLIENT_FOLDER_NAME/$FILE5
fi
if [ -f "$SERVER_FOLDER_NAME/$FILE6" ]; then
	rm $SERVER_FOLDER_NAME/$FILE6
fi
