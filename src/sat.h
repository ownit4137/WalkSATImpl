#ifndef SAT_INSTANCE_H
#define SAT_INSTANCE_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <tuple>
#include <algorithm>
#include <queue>
#include <math.h>
#include <ctype.h>
#include <chrono>
#include <set>

#include <sys/ioctl.h>
#include <unistd.h>

typedef int lit;
typedef int cls;

#define NUM_COLORERS 16

//ykchoi
#define MAX_TRIES 10
#define MAX_FLIPS 1000000
//#define DEBUG
#define DBG_PRINT

class SAT_KCS{
	private:
	public:
		inst(std::string path);
		//kcs
		std::vector<std::vector<int> > ClauseList;
		std::vector<int> ClauseCost;
		std::vector<std::vector<int> > LiteralList;		// number of Clause where each literal is in
		void WalkSat();
		void PrintClauseList(){
			for(size_t c = 0; c < ClauseList.size(); c++ ){
				std::cout << "Clause " << c+1 << ": ";
				for(size_t l = 0; l < ClauseList[c].size(); l++ ){
					std::cout << ClauseList[c][l] << " ";
				}
				std::cout << std::endl;
			}	
		}
		void PrintLiteralList(){
			for(size_t l = 0; l < LiteralList.size(); l++ ){
				std::cout << "Literal " << l+1 << ": ";
				for(size_t c = 0; c < LiteralList[l].size(); c++ ){
					std::cout << LiteralList[l][c] << " ";
				}
				std::cout << std::endl;
			}	
		}
		
};

#endif
