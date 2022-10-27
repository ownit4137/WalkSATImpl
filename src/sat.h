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
#define MAX_TRIES 1
#define MAX_FLIPS 10
#define RAND_P 567
#define DEBU



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
		// [0, ncls-1]에 lit +-[1, nvar]
		std::vector<std::vector<cls>> VarInClause;		// The number of clauses where each literal is in
		// [0, nvar-1]에 cls +-[1, ncls]
		std::vector<int> ClauseCost;					// The number of true literals
		// [0, ncls-1]
		std::vector<bool> answer;
		
		// std::vector<int> PosInUCB;
		// int nextPos;
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
					//std::cout << ClauseInfo[c][l].number << " ";
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
