CFLAGS = -g -Wall -std=c++11

test.out: main.o kcs.o
	g++ $(CFLAGS) -o test.out main.o kcs.o

main.o: main.cpp sat.h
	g++ $(CFLAGS) -c -o main.o main.cpp

kcs.o: kcs.cpp sat.h
	g++ $(CFLAGS) -c -o kcs.o kcs.cpp

clean:
	rm -f *.o *.out

run:
	./test.out ../test/n_queens32.dimacs 0

