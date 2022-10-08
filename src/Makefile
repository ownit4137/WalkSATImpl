test.out: main.o kcs.o
	g++ -o test.out main.o kcs.o

main.o: main.cpp sat.h
	g++ -c -o main.o main.cpp

kcs.o: kcs.cpp sat.h
	g++ -c -o kcs.o kcs.cpp

clean:
	rm -f *.o *.out

run:
	./test.out test/n_queens4.dimacs 0