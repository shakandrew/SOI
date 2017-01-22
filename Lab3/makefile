all: hello

hello: main.o helpers.o
	gcc main.o helpers.o -pthread -lrt -o hello

main.o: main.c
	gcc -c main.c -pthread -lrt

helpers.o: helpers.c
	gcc -c helpers.c -pthread -lrt


clean:
	rm -rf *.o hello
