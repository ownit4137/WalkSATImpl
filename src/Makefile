CFLAGS = -g -Wall -std=c++11
MAKEFLAGS += --silent
#filepath = ../../5sat500.train/* # local
filepath = ../../SAT11/random/large/5SAT/*
# filepath = ../../NoLimits/* # colab

test.out: main.o kcs.o
	g++ $(CFLAGS) -o test.out ./build/main.o ./build/kcs.o

main.o: main.cpp sat.h
	g++ $(CFLAGS) -c -o ./build/main.o main.cpp

kcs.o: kcs.cpp sat.h
	g++ $(CFLAGS) -c -o ./build/kcs.o kcs.cpp

clean:
	rm -f ./build/*.o *.out

run:
#	./test.out ../test/n_queens4_sort.dimacs 0
#	./test.out ../../NoLimits/mp1-21.0.cnf 0
#	./test.out ../../NoLimits/mp1-qpr-bmp280-driver-14.cnf
#	./test.out ../../5sat500.train/unif-k5-r20.00-v500-c10000-S1779740804.cnf
	./test.out ../../SAT11/random/large/5SAT/unif-k5-r20-v2000-c40000-S394483079-002-UNKNOWN.cnf

#	./test.out ../../7sat60.train/unif-k7-r87.79-v60-c5267-S22649540.cnf 
	# for f in $(filepath); do \
	#  	./test.out $$f; \
	# #	head $$f; \
	# done
#	./test.out ../../NoLimits/mp1-9_9.cnf


