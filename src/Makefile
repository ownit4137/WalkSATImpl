CFLAGS = -g -Wall -std=c++11
MAKEFLAGS += --silent
filepath = ../../5sat500.train/* # local
HW = ./solver_hw
SW = ./solver_sw
#filepath = ../../SAT11/random/large/5SAT/*
# filepath = ../../NoLimits/* # colab
# filepath = ../test/*

all: solver_hw solver_sw
solver_sw: main_sw.o kcs_sw.o
	g++ $(CFLAGS) -o solver_sw ./build/main_sw.o ./build/kcs_sw.o

solver_hw: main_hw.o kcs_hw.o
	g++ $(CFLAGS) -o solver_hw ./build/main_hw.o ./build/kcs_hw.o

main_hw.o: main_hw.cpp sat.h
	g++ $(CFLAGS) -c -o ./build/main_hw.o main_hw.cpp

main_sw.o: main_sw.cpp sat.h
	g++ $(CFLAGS) -c -o ./build/main_sw.o main_sw.cpp

kcs_sw.o: kcs.cpp sat.h
	g++ $(CFLAGS) -c -o ./build/kcs_sw.o kcs.cpp

kcs_hw.o: kcs_hw.cpp sat.h
	g++ $(CFLAGS) -c -o ./build/kcs_hw.o kcs_hw.cpp

clean:
	rm -f ./build/*.o solver*

hw:
#	./test.out ../test/sudoku_4_2_2.dimacs 0
#	./test.out ../../NoLimits/mp1-21.0.cnf 0
#	./test.out ../../NoLimits/mp1-qpr-bmp280-driver-14.cnf
#	./test.out ../../uf20-91-inst/uf20-0277.cnf
	./test.out ../../5sat500.train/unif-k5-r20.00-v500-c10000-S1239105171.cnf
#	./test.out ../../SAT11/random/large/5SAT/unif-k5-r20-v2000-c40000-S394483079-002-UNKNOWN.cnf

#	./test.out ../../7sat60.train/unif-k7-r87.79-v60-c5267-S22649540.cnf 
	# for f in $(filepath); do \
	#  	./test.out $$f; \
	# #	head $$f; \
	# done
#	./test.out ../../NoLimits/mp1-9_9.cnf

sw:
	./test_sw.out ../../5sat500.train/unif-k5-r20.00-v500-c10000-S1239105171.cnf

