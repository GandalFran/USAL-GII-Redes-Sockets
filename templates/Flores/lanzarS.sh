killall -9 servidor
rm peticiones.log
make clean
make
./servidor
