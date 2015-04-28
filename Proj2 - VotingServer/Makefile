all:
	gcc -Wall -c err.c
	gcc -Wall -c komisja.c
	gcc -Wall -c raport.c
	gcc -Wall -pthread -c serwer.c
	gcc -Wall err.o komisja.o -o komisja
	gcc -Wall err.o raport.o -o raport
	gcc -Wall -pthread err.o serwer.o -o serwer
	rm -f *.o