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
#include <ctime>

#include <sys/ioctl.h>
#include <unistd.h>

//kcs
#define MAX_TRIES 10000
#define MAX_FLIPS 10000000
#define UCBSIZE 10000
#define PRINT_FLIP_TIMES 10
#define RAND_FLIP 300
#define W1 1
#define W2 0
#define DEBUG_C1
#define DEBUG_I
#define DEBUG_R
#define K 5
#define R 100
#define MAXNCLS
#define MAXNVAR


struct lit {
	// struct alignment? 
	// 1bit : 31bit
	bool varSign;	// 0 for pos 1 for neg
	int varNumber;
};

struct cls {
	// struct alignment? 
	// 1bit : 31bit
	bool varSign;	// 0 for pos 1 for neg
	int clsNumber;
};

class SAT_KCS{
	private:
	public:
		SAT_KCS(std::string path);
		

		std::vector<std::vector<lit>> ClauseInfo;
		// index: [0, ncls-1] element: lit +-[1, nvar]
		std::vector<std::vector<cls>> VarInClause;		// The number of clauses where each literal is in
		// index: [0, nvar-1] element: cls +-[1, ncls]
		std::vector<int> ClauseCost;					// The number of true literals
		// index: [0, ncls-1]
		std::vector<bool> answer;
		
		int numVars, numClauses;
		bool issolved = false;
		double elapsedTime;

		void solve();
		void result();
		void PrintClauseInfo(){
			for(size_t c = 0; c < ClauseInfo.size(); c++ ){
				std::cout << "Clause " << c+1 << ": ";
				for(size_t l = 0; l < ClauseInfo[c].size(); l++ ){
					int n = ClauseInfo[c][l].varSign == 0 ? ClauseInfo[c][l].varNumber : -ClauseInfo[c][l].varNumber;
					std::cout << n << " ";
				}
				std::cout << std::endl;
			}	
		}
		void PrintVarInClause(){
			for(size_t l = 0; l < VarInClause.size(); l++ ){
				std::cout << "Literal " << l+1 << ": ";
				for(size_t c = 0; c < VarInClause[l].size(); c++ ){
					int n = VarInClause[l][c].varSign == 0 ? VarInClause[l][c].clsNumber : -VarInClause[l][c].clsNumber;
					std::cout << n << " ";
				}
				std::cout << std::endl;
			}	
		}
};



#endif