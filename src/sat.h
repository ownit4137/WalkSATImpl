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

struct literalAssignment{
	lit literal;
	bool isFlipped;
	bool isDecided;
};

struct colorAssignment{
	lit literal;
	cls clause;
	bool isSolved;
};

struct offsetMetaData{
	uint64_t position;
	uint64_t size;
};

struct clauseAllocationMetaData{
	uint64_t numClausesAllocated;
	uint64_t startingExtraElements;
	uint64_t clauseStoreElementsFree;
	uint64_t clauseStoreElementsUsed;
	uint64_t longestClauseLength;
};

template <typename T> class MemoryWatchArray{
	private:
		T* ptr;
		uint64_t elements;
		uint64_t accessCount[2];
	public:
		MemoryWatchArray(){
			ptr = nullptr;
			elements = 0;
			accessCount[0] = 0;
			accessCount[1] = 0;
		};

		MemoryWatchArray(uint64_t elements){
			ptr = new T[elements];
			this->elements = elements;
			accessCount[0] = 0;
			accessCount[1] = 0;
		}

		T& operator[](uint64_t index){
			accessCount[0]++; 
			if(accessCount[0] == std::numeric_limits<uint64_t>::max()){
				accessCount[0] = 0;
				accessCount[1]++;
			}else{
				accessCount[0]++;
			}
			return ptr[index];
		}

		T& operator++(){
			accessCount[0]++; 
			if(accessCount[0] == std::numeric_limits<uint64_t>::max()){
				accessCount[0] = 0;
				accessCount[1]++;
			}else{
				accessCount[0]++;
			}
			(*ptr)++;
			return *ptr;
		}

		T& operator--(){
			accessCount[0]++; 
			if(accessCount[0] == std::numeric_limits<uint64_t>::max()){
				accessCount[0] = 0;
				accessCount[1]++;
			}else{
				accessCount[0]++;
			}
			(*ptr)--;
			return *ptr;
		}
	
		T& operator+=(const T& value){
			accessCount[0]++; 
			if(accessCount[0] == std::numeric_limits<uint64_t>::max()){
				accessCount[0] = 0;
				accessCount[1]++;
			}else{
				accessCount[0]++;
			}
			(*ptr) += value;
			return *ptr; 
		}

		T& operator-=(const T& value){
			accessCount[0]++; 
			if(accessCount[0] == std::numeric_limits<uint64_t>::max()){
				accessCount[0] = 0;
				accessCount[1]++;
			}else{
				accessCount[0]++;
			}
			(*ptr) += value;
			return *ptr; 
		}

		MemoryWatchArray& operator=(const MemoryWatchArray& value){
			if(ptr != nullptr){
				delete[] ptr;
			}
			ptr = new T[value.elements];
			elements = value.elements;
			accessCount[0] = value.accessCount[0];
			accessCount[1] = value.accessCount[1];

			for(uint64_t i = 0; i < elements; i++){
				ptr[i] = value.ptr[i];
			}
			return *this;
		}

		const T* getPtr(){
			return (const T*)ptr;
		}

		void resetCount(){
			accessCount[0] = 0;
			accessCount[1] = 0;
		}

		void getAccessCount(uint64_t store[2]){
			store[0] = accessCount[0];
			store[1] = accessCount[1];
		}

		void setAccessCount(uint64_t store[2]){
			accessCount[0] = store[0];
			accessCount[1] = store[1];
		}

		uint64_t getSize(){
			return sizeof(T) * elements;
		}

		~MemoryWatchArray(){
			if(ptr != nullptr){
				delete[] ptr;
			}
		}
};

enum ARRAY_NAME{CLAUSE_STORE, SUM_OF_CLAUSES, CLAUSE_SOLVE_STATE, CLAUSE_COUNTER, LITERAL_STORE, CLAUSE_STORE_OFFSETS, LITERAL_STORE_OFFSETS};

class SATInstance{
	private:
		clauseAllocationMetaData CAMDObject;
		MemoryWatchArray<cls> clauseStore;
		MemoryWatchArray<int64_t> sumOfClauses;
		MemoryWatchArray<unsigned int> clauseSolveState;
		MemoryWatchArray<int> clauseCounter;
		unsigned int unsolvedClauses;

		std::vector<unsigned int> unsatClauseCount;
	
		MemoryWatchArray<lit> literalStore;
		MemoryWatchArray<lit> inferOverflow;
		MemoryWatchArray<bool> isInInferOverflow;

		//literal, was flipped
		MemoryWatchArray<literalAssignment> literalStack;
		MemoryWatchArray<bool> isInLiteralStack;

		unsigned int literalStackHeight;
		unsigned int inferOverflowRemainder;

		MemoryWatchArray<offsetMetaData> clauseStoreOffsets;
		MemoryWatchArray<offsetMetaData> literalStoreOffsets;

		unsigned int numClauses;
		unsigned int numLiterals;
		uint64_t unitClauses = 0;

		unsigned int totalClausesSize;
		unsigned int totalLiteralsSize;

		uint64_t colorUtilization[2][NUM_COLORERS] = {{0}};
		uint64_t backTrackCounter = 0;
		uint64_t backTrackMulti = 0;
		uint64_t decideCounter = 0;

		bool initialized;

		uint64_t stepNumber = 0;
		uint64_t stepNumberRefresh = 0;

		#ifdef DBG_PRINT
			void printMessage(std::string message);
		#endif
		void performResize();
		void printStats();
		void printSize();

		void colorLiteral(std::queue<colorAssignment>* colorAnswer, literalAssignment litToColor, int idle);
		void mergeStreams(std::queue<colorAssignment>* mergeColorAnswer, std::queue<colorAssignment>* colorAnswer);
		void updateStates(std::queue<colorAssignment>* colorAnswer, 
			literalAssignment litToColor[NUM_COLORERS], unsigned int numLitToColor, 
			bool& doBackTrack);

		void undoStates(std::queue<colorAssignment>* colorAnswer);

		void decide(int decisionPolicy, literalAssignment litToColor[NUM_COLORERS], int& numLitToColor);

	public:
		SATInstance(std::string path);
		bool solve(int decisionPolicy);
		void print();

		//ykchoi
		std::vector<std::vector<int> > ClauseList;
		std::vector<int> ClauseCost;
		std::vector<std::vector<int> > LiteralList;
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
