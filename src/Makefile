CFLAGS = -g -Wall -std=c++11
MAKEFLAGS += --silent
filepath = ../../5sat500.train/* # local
HW = ./solver_hw
SW = ./solver_sw
#filepath = ../../SAT11/random/large/5SAT/*
# filepath = ../../NoLimits/* # colab
# filepath = ../test/*
filepath = ../../ai/hoos/Shortcuts/UF250.1065.100/*

all: solver_hw solver_sw
$(SW): main_sw.o kcs_sw.o
	g++ $(CFLAGS) -o $(SW) ./build/main_sw.o ./build/kcs_sw.o

$(HW): main_hw.o kcs_hw.o
	g++ $(CFLAGS) -o $(HW) ./build/main_hw.o ./build/kcs_hw.o

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
#	$(HW) ../test/sudoku_4_2_2.dimacs 0
#	$(HW) ../../NoLimits/mp1-21.0.cnf 0
#	$(HW) ../../NoLimits/mp1-qpr-bmp280-driver-14.cnf
#	$(HW) ../../uf20-91-inst/uf20-0277.cnf
#	$(HW) ../../5sat500.train/unif-k5-r20.00-v500-c10000-S1239105171.cnf
#	$(HW) ../../sat2018/fla-qhid-200-5.cnf
#	$(HW) ../../sat2018/unif-p12-k5-r21.117-v250-c5279-S9174904490903161459.cnf
#	$(HW) ../../sat2018/unif-k3-r3.98-v1000000-c3980000-S9064153533012388819.cnf
#	$(HW) ../../SAT11/random/large/5SAT/unif-k5-r20-v2000-c40000-S394483079-002-UNKNOWN.cnf


#	$(HW) ../../7sat60.train/unif-k7-r87.79-v60-c5267-S22649540.cnf 
	for f in $(filepath); do \
	 	$(HW) $$f; \
	#	head $$f; \
	done
#	$(HW) ../../NoLimits/mp1-9_9.cnf

sw:
#	$(SW) ../../5sat500.train/unif-k5-r20.00-v500-c10000-S1239105171.cnf
	$(SW) ../../sat2018/fla-qhid-200-5.cnf
#	$(SW) ../../sat2018/unif-p12-k5-r21.117-v250-c5279-S9174904490903161459.cnf
#	$(SW) ../../sat2018/unif-k3-r3.98-v1000000-c3980000-S9064153533012388819.cnf
