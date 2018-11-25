make clean
make
killall -9 servidor
rm peticiones.log
./servidor

./cliente localhost TCP ordenes.txt
./cliente localhost TCP ordenes1.txt
./cliente localhost TCP ordenes.txt
./cliente localhost TCP ordenes1.txt
./cliente localhost TCP ordenes.txt
./cliente localhost TCP ordenes.txt
./cliente localhost TCP ordenes1.txt
./cliente localhost TCP ordenes.txt
./cliente localhost TCP ordenes2.txt
./cliente localhost TCP ordenes.txt
./cliente localhost TCP ordenes1.txt
./cliente localhost TCP ordenes2.txt
./cliente localhost TCP ordenes.txt
./cliente localhost TCP ordenes2.txt
./cliente localhost TCP ordenes.txt
./cliente localhost TCP ordenes2.txt
./cliente localhost TCP ordenes.txt
./cliente localhost TCP ordenes2.txt
