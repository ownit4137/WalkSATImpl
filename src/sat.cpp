#include "sat.h"

// hello
SATInstance::SATInstance(std::string path){
	std::vector<cls>* vectorClauseStore;
	std::vector<lit>* vectorLiteralStore;

	unsigned int parseState = 0;
	unsigned int clauseCounted = 0;
	std::ifstream fileDIMACS(path);

	if(fileDIMACS.is_open()){
		std::string line;

		//Parse
		while(getline(fileDIMACS, line)){

			size_t pos = 0;
			std::string delimiter = " ";
			std::string token;

			std::vector<std::string> parseAnswer;

			while((pos = line.find(delimiter)) != std::string::npos){
				
				token = line.substr(0, pos);
				line.erase(0, pos + delimiter.length());

				parseAnswer.push_back(token);
			}
			parseAnswer.push_back(line);

			switch(parseState){
				//Metadata from DIMACS
				case 0:
					numClauses = std::stoi(parseAnswer[parseAnswer.size()-1]);
					numLiterals = std::stoi(parseAnswer[parseAnswer.size()-2]);

					vectorClauseStore = new std::vector<cls>[numClauses];
					vectorLiteralStore = new std::vector<lit>[numLiterals];

					//ykchoi
					{
						std::vector<int> new_clause; 
						for(size_t i = 0; i < numClauses; i++ ){
							//new_clause = new std::vector<int>; 
							ClauseList.push_back(new_clause);
							ClauseCost.push_back(0);
						}

						std::vector<int> new_literal; 
						for(size_t i = 0; i < numLiterals; i++ ){
							//new_clause = new std::vector<int>; 
							LiteralList.push_back(new_literal);
						}

					}

					parseState++;
					break;
				//Store clauses and literals
				case 1:
					{
						//Skip lines starting with 'C'
						bool skipComment = false;
						for(unsigned int i = 0; i < parseAnswer[0].size(); i++){
							if(!isdigit(parseAnswer[0][i]) && parseAnswer[0][i] != '-'){
								skipComment = true;
								break;
							}
						}
						if(skipComment){
							break;
						}

						if(std::stoi(parseAnswer[parseAnswer.size()-1]) != 0){
							fileDIMACS.close();
							initialized = false;
						}else{
							parseAnswer.pop_back();

							for(unsigned int i = 0; i < parseAnswer.size(); i++){
								lit literalVal = std::stoi(parseAnswer[i]);
								if(abs(literalVal) > numLiterals){
									fileDIMACS.close();
									std::cerr << "Literal value and max literal mismatch: " << literalVal << " " << numLiterals << "\n";
									initialized = false;
								}else{
									//ykchoi
									ClauseList[clauseCounted].push_back(literalVal);
									if(literalVal > 0){
										LiteralList[literalVal-1].push_back(clauseCounted+1);
									}else{
										LiteralList[-literalVal-1].push_back(-clauseCounted-1);
									}

									vectorClauseStore[clauseCounted].push_back(literalVal);
									if(literalVal > 0){
										vectorLiteralStore[abs(literalVal)-1].push_back(clauseCounted+1);
									}else{
										vectorLiteralStore[abs(literalVal)-1].push_back(-clauseCounted-1);
									}
								}
							}
				
							clauseCounted++;
						
						}
					}
					break;
				default:
					fileDIMACS.close();
					std::cerr << "Wrong state: " << parseState << "\n";
					initialized = false;
			}

		}	

		fileDIMACS.close();
		if(clauseCounted != numClauses){
			std::cerr << "Clause counter and counted mismatch: " << clauseCounted << " " << numClauses << "\n";
			initialized = false;
		}

		//Sort literal and clauses vector
		auto compare = [](int a, int b){
			int abs_a = abs(a), abs_b = abs(b);
			if (abs_a < abs_b) return true;
			if (abs_b < abs_a) return false;
			return a < b;
		};

		totalClausesSize = 0;
		for(unsigned int i = 0; i < numClauses; i++){
			std::sort(vectorClauseStore[i].begin(), vectorClauseStore[i].end(), compare);
			totalClausesSize += vectorClauseStore[i].size();
		}

		totalLiteralsSize = 0;
		for(unsigned int i = 0; i < numLiterals; i++){
			std::sort(vectorLiteralStore[i].begin(), vectorLiteralStore[i].end(), compare);
			totalLiteralsSize += vectorLiteralStore[i].size();
		}

		//Each Clause and literal in their respective store needs to be aligned to a pow2
		bool addOne = abs(ceil(log2((double)numClauses)) - floor(log2((double)numClauses))) < 0.0001 ? true : false;
		uint64_t numClausePow2 = (uint64_t)pow(2,ceil(log2(numClauses))+addOne);

		sumOfClauses = MemoryWatchArray<int64_t>(numClausePow2);
		clauseSolveState = MemoryWatchArray<unsigned int>(numClausePow2);
		clauseCounter = MemoryWatchArray<int>(numClausePow2);
		clauseStoreOffsets = MemoryWatchArray<offsetMetaData>(numClausePow2);
		unsolvedClauses = numClauses;
	
		std::vector<cls> vectorClauseStore1D;
		unsigned int tmpPosition = 0;
		uint64_t longestClauseLength = 0;


		unsatClauseCount = std::vector<unsigned int>(numClauses);
		for(unsigned int i = 0; i < numClauses; i++){
			sumOfClauses[i] = 0;
			clauseCounter[i] = vectorClauseStore[i].size();
			clauseStoreOffsets[i] = (offsetMetaData){.position=tmpPosition,.size=vectorClauseStore[i].size()};
			clauseSolveState[i] = 0;

			if(vectorClauseStore[i].size() > longestClauseLength){
				longestClauseLength = vectorClauseStore[i].size();
			}

			for(unsigned int j = 0; j < vectorClauseStore[i].size(); j++){
				vectorClauseStore1D.push_back(vectorClauseStore[i][j]);
				sumOfClauses[i] += vectorClauseStore[i][j];
				tmpPosition++;
			}
			unsatClauseCount[i] = 0;
		}

		for(unsigned int i = 0; i < 4096; i++){
			vectorClauseStore1D.push_back(0);
		}

		clauseStore = MemoryWatchArray<cls>(vectorClauseStore1D.size());
		for(unsigned int i = 0; i < vectorClauseStore1D.size(); i++){
			clauseStore[i] = vectorClauseStore1D[i];
		}

		CAMDObject.numClausesAllocated = numClausePow2;
		CAMDObject.startingExtraElements = 4096;
		CAMDObject.clauseStoreElementsFree = 4096;
		CAMDObject.clauseStoreElementsUsed = vectorClauseStore1D.size()-4096;
		CAMDObject.longestClauseLength = longestClauseLength;

		inferOverflow = MemoryWatchArray<lit>(numLiterals);
		isInInferOverflow = MemoryWatchArray<bool>(numLiterals);
		literalStack = MemoryWatchArray<literalAssignment>(numLiterals);
		isInLiteralStack = MemoryWatchArray<bool>(numLiterals);
		literalStoreOffsets = MemoryWatchArray<offsetMetaData>(numLiterals+1);
		inferOverflowRemainder = 0;
		literalStackHeight = 0;

		std::vector<lit> vectorLiteralStore1D;
		tmpPosition = 0;

		for(unsigned int i = 0; i < numLiterals; i++){
			literalStoreOffsets[i] = (offsetMetaData){.position=tmpPosition,.size=vectorLiteralStore[i].size()};
			inferOverflow[i] = 0;
			isInLiteralStack[i] = false;
			isInInferOverflow[i] = false;
		
			for(unsigned int j = 0; j < vectorLiteralStore[i].size(); j++){
				vectorLiteralStore1D.push_back(vectorLiteralStore[i][j]);
				tmpPosition++;		
			}

			uint64_t roundNextPow2 = pow(2,4);
			if(roundNextPow2 < vectorLiteralStore[i].size()){
				uint64_t getPower = (uint64_t)ceil(log2(vectorLiteralStore[i].size()));
				if(vectorLiteralStore[i].size() % (uint64_t)pow(2,getPower) == 0){
					getPower++;
				}
				roundNextPow2 = (uint64_t)pow(2,getPower);
			}

			roundNextPow2 -= vectorLiteralStore[i].size();
			for(unsigned int j = 0; j < roundNextPow2; j++){
				vectorLiteralStore1D.push_back(0);
				tmpPosition++;
			}
		}
		literalStoreOffsets[numLiterals]= (offsetMetaData){.position=tmpPosition,.size=0};

		literalStore = MemoryWatchArray<lit>(vectorLiteralStore1D.size());
		for(unsigned int i = 0; i < vectorLiteralStore1D.size(); i++){
			literalStore[i] = vectorLiteralStore1D[i];
		}
		
		delete[] vectorClauseStore;
		delete[] vectorLiteralStore;
		initialized = true;
	}else{
		std::cerr << "Cannot open file: " << path << "\n";
		initialized = false;
	}
}

void SATInstance::performResize(){
	//std::cout << "PERFORMING RESIZE" << "\n";

	uint64_t holdCountDuringResize[7][2];

	clauseStore.getAccessCount(holdCountDuringResize[CLAUSE_STORE]);

	sumOfClauses.getAccessCount(holdCountDuringResize[SUM_OF_CLAUSES]);
	clauseSolveState.getAccessCount(holdCountDuringResize[CLAUSE_SOLVE_STATE]);
	clauseCounter.getAccessCount(holdCountDuringResize[CLAUSE_COUNTER]);
	clauseStoreOffsets.getAccessCount(holdCountDuringResize[CLAUSE_STORE_OFFSETS]);

	literalStore.getAccessCount(holdCountDuringResize[LITERAL_STORE]);
	
	literalStoreOffsets.getAccessCount(holdCountDuringResize[LITERAL_STORE_OFFSETS]);


	if(CAMDObject.clauseStoreElementsFree < CAMDObject.startingExtraElements/2){
		std::vector<cls> clauseStoreVector;

		for(unsigned int i = 0; i < CAMDObject.clauseStoreElementsUsed; i++){
			clauseStoreVector.push_back(clauseStore[i]);
		}

		clauseStore = MemoryWatchArray<cls>(CAMDObject.clauseStoreElementsUsed + CAMDObject.clauseStoreElementsFree + CAMDObject.startingExtraElements*2);

		for(unsigned int i = 0; i < CAMDObject.clauseStoreElementsUsed; i++){
			clauseStore[i] = clauseStoreVector[i];
		}

		CAMDObject.clauseStoreElementsFree += CAMDObject.startingExtraElements;
		CAMDObject.startingExtraElements *= 2;
	}

	if(numClauses > CAMDObject.numClausesAllocated/2){
		CAMDObject.numClausesAllocated *= 2;
		
		std::vector<int64_t> sumOfClausesVector;
		std::vector<unsigned int> clauseSolveStateVector;
		std::vector<int> clauseCounterVector;
		std::vector<offsetMetaData> clauseStoreOffsetsVector;

		for(unsigned int i = 0; i < numClauses; i++){
			sumOfClausesVector.push_back(sumOfClauses[i]);
			clauseSolveStateVector.push_back(clauseSolveState[i]);
			clauseCounterVector.push_back(clauseCounter[i]);
			clauseStoreOffsetsVector.push_back(clauseStoreOffsets[i]);
		}

		sumOfClauses = MemoryWatchArray<int64_t>(CAMDObject.numClausesAllocated);
		clauseSolveState = MemoryWatchArray<unsigned int>(CAMDObject.numClausesAllocated);
		clauseCounter = MemoryWatchArray<int>(CAMDObject.numClausesAllocated);
		clauseStoreOffsets = MemoryWatchArray<offsetMetaData>(CAMDObject.numClausesAllocated);

		for(unsigned int i = 0; i < numClauses; i++){
			sumOfClauses[i] = sumOfClausesVector[i];
			clauseSolveState[i] = clauseSolveStateVector[i];
			clauseCounter[i] = clauseCounterVector[i];
			clauseStoreOffsets[i] = clauseStoreOffsetsVector[i];
		}
	}

	std::vector<lit>* literalStoreVector = new std::vector<lit>[numLiterals];
	for(unsigned int i = 0; i < numLiterals; i++){
		for(unsigned int j = 0; j < literalStoreOffsets[i+1].position-literalStoreOffsets[i].position; j++){
			literalStoreVector[i].push_back(literalStore[literalStoreOffsets[i].position+j]);
		}

		if(literalStoreOffsets[i].size > (literalStoreOffsets[i+1].position-literalStoreOffsets[i].position)/2){
			for(unsigned int j = 0; j < literalStoreOffsets[i+1].position-literalStoreOffsets[i].position; j++){
				literalStoreVector[i].push_back(0);
			}
		}
	}

	std::vector<lit> vectorLiteralStore1D;
	uint64_t tmpPosition = 0;
	for(unsigned int i = 0; i < numLiterals; i++){
		literalStoreOffsets[i] = (offsetMetaData){.position=tmpPosition,.size=literalStoreOffsets[i].size};

		for(unsigned int j = 0; j < literalStoreVector[i].size(); j++){
			vectorLiteralStore1D.push_back(literalStoreVector[i][j]);
			tmpPosition++;		
		}
	}
	literalStoreOffsets[numLiterals] = (offsetMetaData){.position=tmpPosition,.size=0};

	literalStore = MemoryWatchArray<lit>(vectorLiteralStore1D.size());
	for(unsigned int i = 0; i < vectorLiteralStore1D.size(); i++){
		literalStore[i] = vectorLiteralStore1D[i];
	}
	delete[] literalStoreVector;

	clauseStore.setAccessCount(holdCountDuringResize[CLAUSE_STORE]);
	sumOfClauses.setAccessCount(holdCountDuringResize[SUM_OF_CLAUSES]);
	clauseSolveState.setAccessCount(holdCountDuringResize[CLAUSE_SOLVE_STATE]);
	clauseCounter.setAccessCount(holdCountDuringResize[CLAUSE_COUNTER]);
	literalStore.setAccessCount(holdCountDuringResize[LITERAL_STORE]);
	clauseStoreOffsets.setAccessCount(holdCountDuringResize[CLAUSE_STORE_OFFSETS]);
	literalStoreOffsets.setAccessCount(holdCountDuringResize[LITERAL_STORE_OFFSETS]);

	//print();
}

void SATInstance::print(){
	std::cout << "Clause Store: " << "\n";
	
	for(unsigned int i = 0; i < numClauses; i++){
		std::cout << i+1 << ": ";
		unsigned int tmpPosition = 0;
		while(true){
			std::cout << clauseStore[clauseStoreOffsets[i].position+tmpPosition] << " ";
			
			tmpPosition++;
			if(tmpPosition == clauseStoreOffsets[i].size){
				break;
			}
		}
		std::cout << "\n";
	}


	std::cout << "\n" << "Literal Store: " << "\n";
	for(unsigned int i = 0; i < numLiterals; i++){
		std::cout << i+1 << "(" << literalStoreOffsets[i].position << ")(" << literalStoreOffsets[i].size << "): ";
		unsigned int tmpPosition = 0;
		while(true){
			std::cout << literalStore[literalStoreOffsets[i].position + tmpPosition] << " ";
			
			tmpPosition++;
			if(tmpPosition == literalStoreOffsets[i].size){
				break;
			}
		}
		std::cout << "\n";
	}
	std::cout << "\n";
}

#ifdef DBG_PRINT
void SATInstance::printMessage(std::string message){
	std::cout << "Step Number: " << std::to_string(stepNumberRefresh) << " " << std::to_string(stepNumber) << " " << message;
}
#endif

void SATInstance::printStats(){
	uint64_t holdCount[2];
	std::cout << "Memory access count: " << "\n";
	clauseStore.getAccessCount(holdCount);
	std::cout << "Clause Store: " << holdCount[1] << " " << holdCount[0] << "\n";
	sumOfClauses.getAccessCount(holdCount);
	std::cout << "Sum of Clause: " << holdCount[1] << " " << holdCount[0] << "\n";
	clauseSolveState.getAccessCount(holdCount);
	std::cout << "Clause Solve State: " << holdCount[1] << " " << holdCount[0] << "\n";
	clauseCounter.getAccessCount(holdCount);
	std::cout << "Clause Counter: " << holdCount[1] << " " << holdCount[0] << "\n";
	literalStore.getAccessCount(holdCount);
	std::cout << "Literal Store: " << holdCount[1] << " " << holdCount[0] << "\n";
	inferOverflow.getAccessCount(holdCount);
	std::cout << "Infer Overflow: " << holdCount[1] << " " << holdCount[0] << "\n";
	isInInferOverflow.getAccessCount(holdCount);
	std::cout << "isInInfer Overflow: " << holdCount[1] << " " << holdCount[0] << "\n";
	literalStack.getAccessCount(holdCount);
	std::cout << "Literal Stack: " << holdCount[1] << " " << holdCount[0] << "\n";
	isInLiteralStack.getAccessCount(holdCount);
	std::cout << "isInLiteral Stack: " << holdCount[1] << " " << holdCount[0] << "\n";
	clauseStoreOffsets.getAccessCount(holdCount);
	std::cout << "Clause Store Offset: " << holdCount[1] << " " << holdCount[0] << "\n";
	literalStoreOffsets.getAccessCount(holdCount);
	std::cout << "Literal Store Offset: " << holdCount[1] << " " << holdCount[0] << "\n";

	std::cout << "Colorer utilization Prop/Decide: " << "\n";
	for(unsigned int i = 0; i < NUM_COLORERS; i++){
		std::cout << colorUtilization[0][i] << " ";
	}
	std::cout << "\n";
	std::cout << "Colorer utilization Backtrack: " << "\n";
	for(unsigned int i = 0; i < NUM_COLORERS; i++){
		std::cout << colorUtilization[1][i] << " ";
	}

	unsigned int max = 0;
	unsigned int clauseID = 0;

	for(unsigned int i = 0; i < unsatClauseCount.size(); i++){
		if(max < unsatClauseCount[i]){
			max = unsatClauseCount[i];
			clauseID = i+1;
		}
	}

	std::cout << "\n";
	std::cout << "Backtracked: " << backTrackCounter << " " << backTrackMulti << " Decide: " << decideCounter << " Hot Clause Unsat: " << clauseID << " " << max << "\n";
}

void SATInstance::printSize(){
	std::cout << "Size in Bytes: " << "\n";
	std::cout << "Clause Store: " << clauseStore.getSize() << "\n";
	std::cout << "Sum of Clause: " << sumOfClauses.getSize() << "\n";
	std::cout << "Clause Solve State: " << clauseSolveState.getSize() << "\n";
	std::cout << "Clause Counter: " << clauseCounter.getSize() << "\n";
	std::cout << "Literal Store: " << literalStore.getSize() << "\n";
	std::cout << "Infer Overflow: " << inferOverflow.getSize() << "\n";
	std::cout << "isInInfer Overflow: " << isInInferOverflow.getSize() << "\n";
	std::cout << "Literal Stack: " << literalStack.getSize() << "\n";
	std::cout << "isInLiteral Stack: " << isInLiteralStack.getSize() << "\n";
	std::cout << "Clause Store Offset: " << clauseStoreOffsets.getSize() << "\n";
	std::cout << "Literal Store Offset: " << literalStoreOffsets.getSize() << "\n\n";
}

//ykchoi
void SATInstance::WalkSat(){
#ifdef DEBUG
	PrintClauseList();
	PrintLiteralList();
#endif

//#ifdef DEBUG
	long long ClauseCnt = 0;
	long long LiteralCnt = 0;
	long long unsat_push = 0;
	long long unsat_pop = 0;
//#endif

	srand(0);

	std::chrono::steady_clock::time_point timeStart = std::chrono::steady_clock::now();

	std::set<int> unsat_list;

	for(size_t t = 0; t < MAX_TRIES; t++){
		std::vector<bool> var_assignment;
		for(size_t i = 0; i < numLiterals; i++){
			var_assignment.push_back( rand()%2 );
		}
			
		for(size_t c = 0; c < ClauseList.size(); c++ ){
			ClauseCost[c] = 0;

			for(size_t l = 0; l < ClauseList[c].size(); l++ ){
				bool isPosLit = (ClauseList[c][l] >= 0);
				int var_idx = (isPosLit) ? ClauseList[c][l] : -ClauseList[c][l];
				bool var_value = var_assignment[var_idx-1];
				if( isPosLit == var_value ){
					ClauseCost[c]++;
				}
			}
			if( ClauseCost[c] == 0 ){
				unsat_list.insert(c);
			}
#ifdef DEBUG
			std::cout << "Cost of clause " << c+1 << " is " << ClauseCost[c] << std::endl;
#endif
			ClauseCnt += ClauseList[c].size() + 1;
		}


		for(size_t f = 0; f < MAX_FLIPS; f++){
			int random_clause_idx = 0;
			int random_clause_sel = rand() % unsat_list.size();
			int cnt = 0;
			std::set<int>::iterator it;
			for( cnt = 0, it = unsat_list.begin(); it != unsat_list.end(); it++, cnt++ ){
				if( cnt == random_clause_sel ){
					random_clause_idx = *it;
				}
			}
#ifdef DEBUG
			std::cout << "Clause " << random_clause_idx << "chosen." << std::endl;
#endif

			int random_lit_idx = 0;
			std::vector<int> lit_candidates;
			
			int min_literal = 0;
			int min_break_cnt = numClauses;

			//if( rand() > 0.2*RAND_MAX ){
				for(size_t l = 0; l < ClauseList[random_clause_idx].size(); l++ ){
					bool isPosLit = (ClauseList[random_clause_idx][l] >= 0);
					int var_idx = (isPosLit) ? ClauseList[random_clause_idx][l] : -ClauseList[random_clause_idx][l];
					int break_cnt = 0;

					for(size_t c = 0; c < LiteralList[var_idx-1].size(); c++){
						int clause_idx = (LiteralList[var_idx-1][c] > 0) ? LiteralList[var_idx-1][c] : -LiteralList[var_idx-1][c];
						if( (var_assignment[var_idx-1] == true && 
							LiteralList[var_idx-1][c] > 0) ||	
							(var_assignment[var_idx-1] == false && LiteralList[var_idx-1][c] < 0) ){
						}
						else{
							if( ClauseCost[clause_idx-1] - 1 == 0 ){
								break_cnt++;
							}
						}
					}
					if( break_cnt == 0 ){
						lit_candidates.push_back(l);
					}
					else if( break_cnt < min_break_cnt ){
						min_break_cnt = break_cnt;
						min_literal = l;
					}
					
#ifdef DEBUG
					std::cout << "Literal " << l << " (" << var_idx << ") has " << break_cnt << " break counts." << std::endl;
#endif
					LiteralCnt += LiteralList[var_idx-1].size() + 1;
					ClauseCnt += 1;
				}
			//}


			if( lit_candidates.size() == 0 ){ //there are no literals with 0 breaks
				if( rand() > 0.2*RAND_MAX ){ //choosing literal with least breaks
					random_lit_idx = min_literal;
#ifdef DEBUG
					std::cout << "Literal " << random_lit_idx << " chosen." << std::endl;
#endif
				}
				else{
					for(size_t l = 0; l < ClauseList[random_clause_idx].size(); l++ ){
						lit_candidates.push_back(l);
					}
				}
			}

			if( lit_candidates.size() != 0 ){ //literal not chosen yet - choose randomly
				random_lit_idx = lit_candidates[rand() % lit_candidates.size()];
#ifdef DEBUG
				std::cout << "Literal " << random_lit_idx << " randomly chosen." << std::endl;
#endif
			}

			bool isPosLit = (ClauseList[random_clause_idx][random_lit_idx] >= 0);
			int var_idx = (isPosLit) ? ClauseList[random_clause_idx][random_lit_idx] : -ClauseList[random_clause_idx][random_lit_idx];
			if( var_assignment[var_idx-1] == true )  var_assignment[var_idx-1] = false;
			else  var_assignment[var_idx-1] = true;

			for(size_t c = 0; c < LiteralList[var_idx-1].size(); c++){
				int clause_idx = (LiteralList[var_idx-1][c] > 0) ? LiteralList[var_idx-1][c] : -LiteralList[var_idx-1][c];
				if( (var_assignment[var_idx-1] == true && LiteralList[var_idx-1][c] > 0) ||	
				(var_assignment[var_idx-1] == false && LiteralList[var_idx-1][c] < 0) ){
					if( ClauseCost[clause_idx-1] == 0 ){
						std::set<int>::iterator it = unsat_list.find(clause_idx-1);
						if( it == unsat_list.end() ){
							std::cout << "Error. Cannot find clause " << clause_idx << std::endl;
							exit(0);					
						}
						unsat_list.erase( it );
//#ifdef DEBUG
						unsat_pop++;
//#endif
					}					
					ClauseCost[clause_idx-1]++;
				}
				else{
					ClauseCost[clause_idx-1]--;
					if( ClauseCost[clause_idx-1] == 0 ){
						unsat_list.insert(clause_idx-1);
//#ifdef DEBUG
						unsat_push++;
//#endif
					}
				}
#ifdef DEBUG
				std::cout << "Cost of clause " << clause_idx << " is " << ClauseCost[clause_idx-1] << std::endl;
#endif
			}
#ifdef DEBUG
			std::cout << "Try : " << t << ", flip : " << f << ", flipped : " << ClauseList[random_clause_idx][random_lit_idx] << " (c:" << random_clause_idx << ",l:" << random_lit_idx << "), unsat_num : " << unsat_list.size() << std::endl;	
#endif
			if( unsat_list.size() == 0 ){
				std::cout << "WalkSAT solution found in " << t << " tries and " << f << " flips. Result: " << std::endl;
				for(size_t i = 0; i < numLiterals; i++){
					if( var_assignment[i] == false ){
						std::cout << "-" ;
					}
					std::cout << i+1 << " ";
				}	
				std::cout << std::endl;

				for(size_t c = 0; c < ClauseList.size(); c++ ){
					bool isSat = false;

					for(size_t l = 0; l < ClauseList[c].size(); l++ ){
						bool isPosLit = (ClauseList[c][l] >= 0);
						int var_idx = (isPosLit) ? ClauseList[c][l] : -ClauseList[c][l];
						bool var_value = var_assignment[var_idx-1];
#ifdef DEBUG
						//std::cout << "c: " << c << " l: " << l << " ClauseList[c][l]: " << ClauseList[c][l] << " isPosLit: " << int(isPosLit) << " var_idx: " << var_idx << " var_value: " << int(var_value) << std::endl;
#endif
						if( isPosLit == var_value ){
							isSat = true;
							break;
						}
					}
					if( isSat == false ){
						std::cout << "Error: Clause " << c << " is not satisfied " << std::endl;
						exit(0);					
					}
				}
			
				break;
			}

		}
		if( unsat_list.size() == 0 ){

			break;
		}
	}

	if( unsat_list.size() != 0 ){
		std::cout << "WalkSAT could not find a solution." << std::endl;
	}

	std::chrono::steady_clock::time_point timeEnd = std::chrono::steady_clock::now();
	double elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count();
	std::cout << "WalkSAT completed in: " << elapsedTime/1000000 << " seconds " << std::endl;

//#ifdef DEBUG
	std::cout << "ClauseCnt: " << ClauseCnt << std::endl;
	std::cout << "LiteralCnt: " << LiteralCnt << std::endl;
	std::cout << "unsat_push: " << unsat_push << std::endl;
	std::cout << "unsat_pop: " << unsat_pop << std::endl;
//#endif

}












bool SATInstance::solve(int decisionPolicy){

	bool doBackTrack = false;
	bool useDecided = false;
	literalAssignment litToColor[NUM_COLORERS];
	int numLitToColor = 0;

	for(unsigned int i = 0; i < NUM_COLORERS; i++){
		litToColor[i].literal = 0;
		litToColor[i].isFlipped = false;
		litToColor[i].isDecided = false;
	}

	//Debug stuff
	std::string message = " ";

	print();
	printSize();

	clauseStore.resetCount();
	sumOfClauses.resetCount();
	clauseSolveState.resetCount();
	clauseCounter.resetCount();

	literalStore.resetCount();
	inferOverflow.resetCount();
	isInInferOverflow.resetCount();

	literalStack.resetCount();
	isInLiteralStack.resetCount();

	clauseStoreOffsets.resetCount();
	literalStoreOffsets.resetCount();

	//For print use
	unsigned int insertUp = 0;

	#ifndef DBG_PRINT
	std::chrono::steady_clock::time_point timeStart = std::chrono::steady_clock::now();
	#endif

	std::chrono::steady_clock::time_point calcStart = std::chrono::steady_clock::now();
	//This while loop is kernel execution inside FPGA
	while(true){
		if(!doBackTrack){
			numLitToColor = 0;
				
			//First get unit clauses - to preserve propagation level order
			if(!useDecided){
				if(inferOverflowRemainder == 0){
					//No inferred literals left, need to decide on one
					//Get an undetermined literal
					//Heuristic needed to select what literal to choose
					decide(decisionPolicy, litToColor, numLitToColor);

				}else{
					unsigned int getOverflowRemainder = inferOverflowRemainder;
					for(unsigned int i = 0; i < getOverflowRemainder; i++){
						litToColor[i] = (literalAssignment){.literal=inferOverflow[inferOverflowRemainder-1],.isFlipped=false,.isDecided=false};

						if(!isInInferOverflow[abs(litToColor[i].literal)-1]){
							litToColor[i].literal = 0;
						}

						inferOverflowRemainder--;
						numLitToColor++;
						if(numLitToColor == NUM_COLORERS){
							break;
						}
					}
				}
			}else{
				numLitToColor = 1;
			}

			colorUtilization[0][numLitToColor-1]++;

			//Do Coloring and merge
			std::queue<colorAssignment> colorAnswer[NUM_COLORERS];
			std::queue<colorAssignment> mergeColorAnswer[15];

			for(unsigned int i = 0; i < NUM_COLORERS; i++){
				colorLiteral(&colorAnswer[i], litToColor[i], numLitToColor-i);
			}
			
			mergeStreams(&mergeColorAnswer[0], colorAnswer);
			mergeStreams(&mergeColorAnswer[1], colorAnswer+2);
			mergeStreams(&mergeColorAnswer[2], colorAnswer+4);
			mergeStreams(&mergeColorAnswer[3], colorAnswer+6);
			mergeStreams(&mergeColorAnswer[4], colorAnswer+8);
			mergeStreams(&mergeColorAnswer[5], colorAnswer+10);
			mergeStreams(&mergeColorAnswer[6], colorAnswer+12);
			mergeStreams(&mergeColorAnswer[7], colorAnswer+14);
			
			mergeStreams(&mergeColorAnswer[8], mergeColorAnswer);
			mergeStreams(&mergeColorAnswer[9], mergeColorAnswer+2);
			mergeStreams(&mergeColorAnswer[10], mergeColorAnswer+4);
			mergeStreams(&mergeColorAnswer[11], mergeColorAnswer+6);

			mergeStreams(&mergeColorAnswer[12], mergeColorAnswer+8);
			mergeStreams(&mergeColorAnswer[13], mergeColorAnswer+10);

			mergeStreams(&mergeColorAnswer[14], mergeColorAnswer+12);
			
			updateStates(&mergeColorAnswer[14], litToColor, numLitToColor, doBackTrack);

			useDecided = false;
		}else{
				//BackTrack
				//Undo stack until decision level
				backTrackCounter++;
				
			//Do Coloring and merge
			std::queue<colorAssignment> colorAnswer[NUM_COLORERS];
			std::queue<colorAssignment> mergeColorAnswer[15];

			numLitToColor = 1;
			for(unsigned int i = 1; i < NUM_COLORERS; i++){
				if(literalStackHeight > 0 && !literalStack[literalStackHeight-1].isDecided){
					litToColor[i] = literalStack[literalStackHeight-1];
						
					isInLiteralStack[abs(litToColor[i].literal)-1] = false;
					literalStackHeight--;
					numLitToColor++;
				}
			}
			litToColor[0] = literalStack[literalStackHeight-1];
			literalStackHeight--;

			colorUtilization[1][numLitToColor-1]++;

			for(unsigned int i = 0; i < NUM_COLORERS; i++){
				colorLiteral(&colorAnswer[i], litToColor[i], numLitToColor-i);
			}
			
			mergeStreams(&mergeColorAnswer[0], colorAnswer);
			mergeStreams(&mergeColorAnswer[1], colorAnswer+2);
			mergeStreams(&mergeColorAnswer[2], colorAnswer+4);
			mergeStreams(&mergeColorAnswer[3], colorAnswer+6);
			mergeStreams(&mergeColorAnswer[4], colorAnswer+8);
			mergeStreams(&mergeColorAnswer[5], colorAnswer+10);
			mergeStreams(&mergeColorAnswer[6], colorAnswer+12);
			mergeStreams(&mergeColorAnswer[7], colorAnswer+14);
			
			mergeStreams(&mergeColorAnswer[8], mergeColorAnswer);
			mergeStreams(&mergeColorAnswer[9], mergeColorAnswer+2);
			mergeStreams(&mergeColorAnswer[10], mergeColorAnswer+4);
			mergeStreams(&mergeColorAnswer[11], mergeColorAnswer+6);

			mergeStreams(&mergeColorAnswer[12], mergeColorAnswer+8);
			mergeStreams(&mergeColorAnswer[13], mergeColorAnswer+10);

			mergeStreams(&mergeColorAnswer[14], mergeColorAnswer+12);

			//Can reset this stack because unit propagation that dead ended
			//is caused by a decided literal
			for(unsigned int i = 0; i < inferOverflowRemainder; i++){
				isInInferOverflow[abs(inferOverflow[i])-1] = false;
			}
			inferOverflowRemainder = 0;
			
			if(litToColor[0].isFlipped && literalStackHeight == 0){
				message = "Instance unsatisfiable \n\n";
				std::cout << message;
				printStats();
				
				std::chrono::steady_clock::time_point calcEnd = std::chrono::steady_clock::now();
				double elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(calcEnd - calcStart).count();
				std::cout << "Completed in: "
					<< elapsedTime/1000000 
					<< " seconds with steps: "
					<< stepNumberRefresh << " " << stepNumber << "\n";

				return false;
			}
			
			//Undo coloring
			undoStates(&mergeColorAnswer[14]);
			if(litToColor[0].isDecided && !litToColor[0].isFlipped){
				litToColor[0].isFlipped = true;
				litToColor[0].literal = -litToColor[0].literal;
				doBackTrack = false;
				useDecided = true;
			}else{
				doBackTrack = true;
			}
		}

		//Satisfiable
		//Correctness check and stat printout
		//In HLS just return true
		if(unsolvedClauses == 0){
			const literalAssignment* literalStackPtr = literalStack.getPtr();
			std::vector<lit> answerResults;

			auto compare = [](int a, int b){
				int abs_a = abs(a), abs_b = abs(b);
				if (abs_a < abs_b) return true;
				if (abs_b < abs_a) return false;
				return a < b;
			};

			for(unsigned int i = 0; i < numLiterals; i++){
				answerResults.push_back(literalStackPtr[i].literal);
			}
			std::sort(answerResults.begin(),answerResults.end(), compare);

			message = "Instance satisfiable \n";
			for(unsigned int i = 0; i < numLiterals; i++){
				message += std::to_string(answerResults[i]) + " ";
			}
			message += "\n\n";
			std::cout << message;
			printStats();

			unsigned int solvedClauses = 0;
			for(unsigned int i = 0; i < numClauses; i++){
				offsetMetaData clauseMetaData = clauseStoreOffsets[i];

				uint64_t clauseLength = clauseMetaData.size;
				uint64_t offset = clauseMetaData.position;
				for(uint64_t j = 0; j < clauseLength; j++){
					lit getLiteralInClause = clauseStore[offset+j];
					if(answerResults[abs(getLiteralInClause)-1] == getLiteralInClause){
						solvedClauses++;
						break;
					}
				}
			}
			if(solvedClauses != numClauses){
				std::cout << "WAS NOT SOLVED PROPERLY: " << solvedClauses << " " << numClauses << "\n";
			}

			std::chrono::steady_clock::time_point calcEnd = std::chrono::steady_clock::now();
			double elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(calcEnd - calcStart).count();
			std::cout << "Completed in: " << elapsedTime/1000000 << " seconds with steps: "
				<< stepNumberRefresh << " " << stepNumber << "\n";

			return true;
		}

		//Step counter for debugging
		if(stepNumber == std::numeric_limits<uint64_t>::max()){
			stepNumber = 0;
			stepNumberRefresh++;
		}else{
			stepNumber++;
		}

		#ifndef DBG_PRINT
		std::chrono::steady_clock::time_point timeEnd = std::chrono::steady_clock::now();
		double elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count();
		if(elapsedTime > 5){
			struct winsize w;
			ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		
			const literalAssignment* literalStackPtr = literalStack.getPtr();

			for(unsigned int i = 0; i < insertUp; i++){
				std::cout << "\33[2K\33[F" << std::flush;
			}
			message = "\nStep Number: " + std::to_string(stepNumberRefresh) + " " + std::to_string(stepNumber) + " ";
			message += "Literal answer stack: (" + std::to_string(literalStackHeight) + ") ";
			for(unsigned int i = 0; i < literalStackHeight; i++){
				message += std::to_string(literalStackPtr[i].literal);
				if(literalStackPtr[i].isDecided){
					message += "(XX) ";
				}else{
					message += " ";	
				}
			}

			insertUp = ceil((double)message.length()/(double)w.ws_col) + 18;
			std::cout << message << "\n" << std::flush;
			printStats();
	
			timeStart = std::chrono::steady_clock::now();
		}
		#endif
	}
}

void SATInstance::colorLiteral(std::queue<colorAssignment>* colorAnswer, literalAssignment litToColor, int idle){
	//Do coloring
	//Replace queue with hls::stream

	if(idle <= 0 || litToColor.literal == 0){
		return;
	}
		
	lit absLiteral = abs(litToColor.literal);
	lit literalInspect = litToColor.literal;

	offsetMetaData literalMetaData = literalStoreOffsets[absLiteral-1];
	unsigned int literalOffsetPos = literalMetaData.position;
	unsigned int literalElements = literalMetaData.size;

	for(unsigned int i = 0; i < literalElements; i++){
		cls clsValue = literalStore[literalOffsetPos+i];
		colorAssignment putOne;
		putOne.literal = literalInspect;
		putOne.clause = clsValue;

		if((clsValue > 0 && literalInspect > 0) || (clsValue < 0 && literalInspect < 0) ){
			putOne.isSolved = true;
			colorAnswer->emplace(putOne);
		}else{
			putOne.isSolved = false;
			colorAnswer->emplace(putOne);
		}
	}
}

void SATInstance::mergeStreams(std::queue<colorAssignment>* mergeColorAnswer, std::queue<colorAssignment>* colorAnswer){
	colorAssignment q0, q1;
	unsigned int lastSent = 2;

	//Merge the streams by sorting by clause id
	while(true){
		//Both queues are empty
		//If both queues are empty at same time, then no need to insert anything
		//Else need to insert the last element from a stream held locally
		if(colorAnswer[0].empty() && colorAnswer[1].empty()){
			if(lastSent == 1){
				mergeColorAnswer->emplace(q0);
			}else if(lastSent == 0){
				mergeColorAnswer->emplace(q1);
			}
			break;
		}

		//If stream0 empty, insert stream1
		//If stream1 empty, insert stream0
		//If both stream not empty, initially grab both and compare
		//Hold onto the element not inserted since it will be used to compare next iteration
		//Grab from the stream that had its element inserted to merge stream 
		if(colorAnswer[0].empty()){
			mergeColorAnswer->emplace(colorAnswer[1].front());
			colorAnswer[1].pop();
		}else if(colorAnswer[1].empty()){
			mergeColorAnswer->emplace(colorAnswer[0].front());
			colorAnswer[0].pop();
		}else{
			if(lastSent == 2){
				q0 = colorAnswer[0].front();
				q1 = colorAnswer[1].front();

				colorAnswer[0].pop();
				colorAnswer[1].pop();
			}else if(lastSent == 1){
				q1 = colorAnswer[1].front();
				colorAnswer[1].pop();
			}else if(lastSent == 0){
				q0 = colorAnswer[0].front();
				colorAnswer[0].pop();
			}

			if(abs(q0.clause) < abs(q1.clause)){
				mergeColorAnswer->emplace(q0);
				lastSent = 0;
			}else{
				mergeColorAnswer->emplace(q1);
				lastSent = 1;
			}
		}
	}
}

void SATInstance::updateStates(std::queue<colorAssignment>* colorAnswer, 
	literalAssignment litToColor[NUM_COLORERS], unsigned int numLitToColor, 
	bool& doBackTrack){
	//Update for literals

	unsigned int tmpClauseSolveState = 0;
	int tmpClauseCounter = 0;
	int64_t tmpSumOfClauses = 0;
	cls prevClauseID = 0;
	bool initClauseID = false;

	while(colorAnswer->size() > 0){
		colorAssignment colorAnswerElement = colorAnswer->front();
		colorAnswer->pop();

		lit literalID = colorAnswerElement.literal;
		cls clauseID = abs(colorAnswerElement.clause)-1;
		bool didSolveClause = colorAnswerElement.isSolved; 

		//Clause solved by literal
		//With mergeStreams - reduce the need to go to memory as much as possible
		//Ordering is by clause
		if(prevClauseID != clauseID || !initClauseID){
			if(prevClauseID != clauseID && initClauseID){
				clauseSolveState[prevClauseID] = tmpClauseSolveState;
				clauseCounter[prevClauseID] = tmpClauseCounter;
				sumOfClauses[prevClauseID] = tmpSumOfClauses;
			}

			tmpClauseSolveState = clauseSolveState[clauseID];
			tmpClauseCounter = clauseCounter[clauseID];
			tmpSumOfClauses = sumOfClauses[clauseID];

			prevClauseID = clauseID;
			initClauseID = true;
		}

		if(didSolveClause){
			if(tmpClauseSolveState == 0){
				unsolvedClauses--;
			}
			tmpClauseSolveState++;
		}

		//State keeping to find last literal and if a clause not satisfied
		if(didSolveClause){
			tmpSumOfClauses -= literalID;
		}else{
			tmpSumOfClauses += literalID;
		}
		tmpClauseCounter--;

		//Clause unsatisfied - prepare backtrack but let update finish
		//Reason is to allow for easier backtrack cleanup
		//If using flipped literal, there will never be more than 1 literal causing unsatisfied
		if(tmpClauseCounter == 0 && tmpClauseSolveState == 0){
			doBackTrack = true;
		}

		//If unit clause found - insert the literal to the todo infer stack
		if(tmpClauseCounter == 1 && tmpClauseSolveState == 0){
			if(!isInInferOverflow[abs(tmpSumOfClauses)-1]){
				inferOverflow[inferOverflowRemainder] = tmpSumOfClauses;
				inferOverflowRemainder++;
				isInInferOverflow[abs(tmpSumOfClauses)-1] = true;
			}
		}
	}

	//Atomic transaction
	//Only remove from inferOverflow all literals processed
	for(unsigned int i = 0; i < numLitToColor; i++){

		if(litToColor[i].literal != 0){
			isInInferOverflow[abs(litToColor[i].literal)-1] = false;

			literalStack[literalStackHeight] = litToColor[i];
			isInLiteralStack[abs(litToColor[i].literal)-1] = true;
			literalStackHeight++;
		}
	}

	if(initClauseID){
		clauseSolveState[prevClauseID] = tmpClauseSolveState;
		clauseCounter[prevClauseID] = tmpClauseCounter;
		sumOfClauses[prevClauseID] = tmpSumOfClauses;
	}	
}

void SATInstance::undoStates(std::queue<colorAssignment>* colorAnswer){

	unsigned int tmpClauseSolveState = 0;
	int tmpClauseCounter = 0;
	int64_t tmpSumOfClauses = 0;
	cls prevClauseID = 0;
	bool initClauseID = false;

	//Undo for a single literal
	while(colorAnswer->size() > 0){
		colorAssignment colorAnswerElement = colorAnswer->front();
		colorAnswer->pop();
		
		lit literalID = colorAnswerElement.literal;
		cls clauseID = abs(colorAnswerElement.clause)-1;
		bool didSolveClause = colorAnswerElement.isSolved;

		//Clause solved by literal
		//With mergeStreams - reduce the need to go to memory as much as possible
		//Ordering is by clause
		if(prevClauseID != clauseID || !initClauseID){
			if(prevClauseID != clauseID && initClauseID){
				clauseSolveState[prevClauseID] = tmpClauseSolveState;
				clauseCounter[prevClauseID] = tmpClauseCounter;
				sumOfClauses[prevClauseID] = tmpSumOfClauses;
			}

			tmpClauseSolveState = clauseSolveState[clauseID];
			tmpClauseCounter = clauseCounter[clauseID];
			tmpSumOfClauses = sumOfClauses[clauseID];

			prevClauseID = clauseID;
			initClauseID = true;
		}

		if(didSolveClause){

			tmpClauseSolveState--;
			if(tmpClauseSolveState == 0){
				unsolvedClauses++;
			}
		}

		if(didSolveClause){
			tmpSumOfClauses += literalID;
		}else{
			tmpSumOfClauses -= literalID;
		}
		tmpClauseCounter++;
	}

	if(initClauseID){
		clauseSolveState[prevClauseID] = tmpClauseSolveState;
		clauseCounter[prevClauseID] = tmpClauseCounter;
		sumOfClauses[prevClauseID] = tmpSumOfClauses;
	}

}

void SATInstance::decide(int decisionPolicy, literalAssignment litToColor[NUM_COLORERS], int& numLitToColor){
	//0 - get a literal by id order

	switch(decisionPolicy){
		case 0:{
			bool foundOne = false;

			numLitToColor = 1;
			for(unsigned int i = 0; i < numLiterals; i++){
				lit getUndecidedLiteral = i+1;
				if(!isInLiteralStack[getUndecidedLiteral-1] && !isInInferOverflow[getUndecidedLiteral-1]){

					offsetMetaData literalMetaData = literalStoreOffsets[getUndecidedLiteral-1];
					unsigned int literalOffsetPos = literalMetaData.position;
					unsigned int literalElements = literalMetaData.size;
		
					unsigned int positiveCount = 0;
					for(unsigned int j = 0; j < literalElements; j++){	
						cls clauseVal = literalStore[literalOffsetPos+j];
						
						if(clauseSolveState[abs(clauseVal)-1] == 0){
							if(clauseVal > 0){
								positiveCount++;
							}
							foundOne = true;
						}
					}
					if(positiveCount > literalElements/2){
						litToColor[0] = (literalAssignment)
							{.literal=getUndecidedLiteral,.isFlipped=false,.isDecided=true};
					}else{
						litToColor[0] = (literalAssignment)
							{.literal=-getUndecidedLiteral,.isFlipped=false,.isDecided=true};
					}

					if(foundOne){
						break;
					}
				}
			}


			decideCounter++;
			break;
		}
		default:
			std::cout << "INVALID DECISION POLICY" << "\n";
			break;
	}
}

