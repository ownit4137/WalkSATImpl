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
#define MAX_FLIPS 500
#define UCBSIZE 100000
#define PRINT_FLIP_TIMES 5
#define RAND_FLIP 300
#define W 0
#define DEBUG_FV
#define K 3
#define R 100
#define MAXNCLS 1000000
#define MAXNVAR 100000
#define MAXUCB 


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

	bool operator != (cls a) {
		return (varSign != a.varSign) || (clsNumber != a.clsNumber);
	}
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
		int maxflip;

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

class WSAT_HW {
	private:
	public:
		WSAT_HW(std::string path);
		void info();
		void ucbInsertHash(int);
		void ucbEraseHash(int);
		void ucbInsertArr(int);
		void ucbEraseArr(int);
		void reset();
		void updateCost();
		void solve();
		void result();
		
		// K is fixed
		lit ClausesVec[MAXNCLS][K];
		// [0, ncls-1]에 lit +-[1, nvar]
		int AddTransT[MAXNVAR];
		// [0, nvar-1]에 int [offset][mask]
		cls VarsLocVec[MAXNVAR];						// The number of clauses where each literal is in, queue, index by at
		// [0, nvar-1]에 cls +-[1, ncls]
		int ClausesCost[MAXNCLS];						// The number of true literals
		// [0, ncls-1]에 int
		// big int UCBArr[MAXNCLS];
		bool answer[MAXNVAR];

		int pndif[MAXNVAR];
		
		bool VATArr[MAXNVAR];	// FPGA -> 1bit
		int UCBArr[MAXNCLS];	// cls +-[0, ncls-1]

		std::vector<std::vector<cls>> VarInClause;		// The number of clauses where each literal is in
		// index: [0, nvar-1] element: cls +-[1, ncls]
		
		
		// std::vector<int> PosInUCB;
		// int nextPos;
		int numVars, numClauses;
		bool issolved = false;
		double elapsedTime;
		int collision = 0;
		int VLVend = 0;
		int multiplier = 1000;
		int ucblast = 0;	// loc next elem or #entries
		int maxflip;

		void printUCB() {
			std::cout << "#cls: " << ucblast << " || ";
			for (int i = 0; i < ucblast; i++) {
				std::cout << UCBArr[i] + 1 << " ";
			}
			std::cout << "\n";
		}
};

#endif