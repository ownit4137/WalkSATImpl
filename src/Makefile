CFLAGS = -g -Wall -std=c++11
# filepath = ../test/* # local
filepath = ../../NoLimits/* # colab

test.out: main.o kcs.o
	g++ $(CFLAGS) -o test.out main.o kcs.o

main.o: main.cpp sat.h
	g++ $(CFLAGS) -c -o main.o main.cpp

kcs.o: kcs.cpp sat.h
	g++ $(CFLAGS) -c -o kcs.o kcs.cpp

clean:
	rm -f *.o *.out

run:
#	./test.out ../test/sudoku_9_3_3.dimacs 0
#	./test.out ../test/g2-ak128astepbg1btisc.cnf 0
	for f in $(filepath); do \
		./test.out $$f; \
	done
