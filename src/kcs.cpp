#include "sat.h"

SAT_KCS::SAT_KCS(std::string path){
	int numClauses, numLiterals;
	std::ifstream fileDIMACS(path);

	std::cout << "Start\n";
	if(fileDIMACS.is_open()){
		std::string line;
		
		// parsing the first line
		fileDIMACS >> line;	// p
		fileDIMACS >> line;	// cnf
		fileDIMACS >> numLiterals;	// nv
		// numLiterals = stoi(line);
		fileDIMACS >> numClauses;	// nc
		// numClauses = stoi(line);

		LiteralList.resize(numLiterals, std::vector<int>(0));
		ClauseList.resize(numClauses, std::vector<int>(0));
		ClauseCost.resize(numClauses);
		std::fill(ClauseCost.begin(), ClauseCost.end(), 0);


		std::string firstLit;
		int firstLitVal;
		int nextLitVal;
		int clauseCounted = 0;
		std::cout << numLiterals << numClauses << " Initialization\n";
		while (!fileDIMACS.eof()) {
			fileDIMACS >> firstLitVal;
			std::cout << clauseCounted <<"st line " << firstLitVal << "\n";
			if (fileDIMACS.eof()) {
				break;
			}
			if (fileDIMACS.bad()) {
				getline(fileDIMACS, line);
				continue;
			}

			ClauseList[clauseCounted].push_back(firstLitVal);
			if (firstLitVal > 0) {
				LiteralList[firstLitVal - 1].push_back(clauseCounted);
			} else {
				LiteralList[-firstLitVal - 1].push_back(-clauseCounted);
			}
			// vectorLiteralStore?


			while (true) {
				fileDIMACS >> nextLitVal;
				if (nextLitVal == 0){
					break;
				}

				ClauseList[clauseCounted].push_back(nextLitVal);
				if (nextLitVal > 0) {
					LiteralList[nextLitVal - 1].push_back(clauseCounted);
				} else {
					LiteralList[-nextLitVal - 1].push_back(-clauseCounted);
				}
			}
			clauseCounted++;
		}
		/*
		while(!fileDIMACS.eof()){
		
			fileDIMACS >> firstLit;
			std::cout << i <<"nst line\n";
			
			// comment
			if (firstLit[0] == 'c'){
				std::cout << "comment\n";
				while (!fileDIMACS.eof()){
					getline(fileDIMACS, line);
				}
				break;
			}

			firstLitVal = firstLit[0] == '-' ? -(firstLit[1] - '0') : fistLit[0];
			ClauseList[i].push_back(firstLitVal);
			LiteralList[firstLitVal].push_back(i);
			
			while (true) {
				fileDIMACS >> nextLitVal;
				if (nextLitVal == 0){
					break;
				}
				ClauseList[i].push_back(nextLitVal);
				LiteralList[nextLitVal].push_back(i);

			}
			i++;
		}
		*/





		/*
		getline(fileDIMACS, line);



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
		*/
	} else {
		std::cerr << "Cannot open file: " << path << "\n";
	}
}