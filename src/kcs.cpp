#include "sat.h"

SAT_KCS::SAT_KCS(std::string path){
	std::ifstream fileDIMACS(path);

	if(fileDIMACS.is_open()){
		std::string line;
		char pp;
		while ((pp = fileDIMACS.peek()) == 'c') {
			fileDIMACS.ignore(256, '\n');
		}
		
		// parsing the first line
		fileDIMACS >> line;	// p
		fileDIMACS >> line;	// cnf
		fileDIMACS >> numVars;	// nv
		fileDIMACS >> numClauses;	// nc

		// std::cout << numVars << "," << numClauses << ",";

		VarInClause.resize(numVars, std::vector<cls>(0));
		ClauseInfo.resize(numClauses, std::vector<lit>(0));
		ClauseCost.resize(numClauses, 0);
		answer.resize(numVars, 0);

		int nextLit;
		int clauseCounted = 1;

		char p;
		while (!fileDIMACS.eof()) {
			fileDIMACS.ignore(256, '\n');
			p = fileDIMACS.peek();
			if (p == EOF || fileDIMACS.eof()) {
				break;
			}
			if (p == 'c') {
				fileDIMACS.ignore(256, '\n');
				continue;
			}
			
			while (true) {
				fileDIMACS >> nextLit;
				if (nextLit == 0){
					break;
				}

				lit tl;
				cls tc;
				tl.varSign = nextLit < 0;
				tl.varNumber = tl.varSign ? -nextLit : nextLit;	// +- 1 ~ nvar
				tc.varSign = nextLit < 0;
				tc.clsNumber = clauseCounted;					// +- 1 ~ ncls

				ClauseInfo[clauseCounted - 1].push_back(tl);
				VarInClause[tl.varNumber - 1].push_back(tc);
			}
			clauseCounted++;
		}
		
		if (clauseCounted - 1 != numClauses) {
			std::cout << "#clauses error\n";
		}

	} else {
		std::cerr << "Cannot open file: " << path << "\n";
	}
}


void SAT_KCS::solve(){
	srand(time(0));
	std::chrono::steady_clock::time_point timeStart = std::chrono::steady_clock::now();
	std::set<int> UCB;	// Unsatisfied Clause Buffer
	// [0, ncls-1]

	// 1 TRY
	for (size_t t = 0; t < MAX_TRIES; t++) {

#ifdef DEBUG_I
		std::cout << "TRY-"<< t + 1 << "\n";
#endif
		std::vector<bool> var_assignment;
		UCB.clear();
		int UCBList[UCBSIZE] = {0, };

		// Initialize
		for (size_t i = 0; i < numVars; i++) {
			int b = rand()%2;
			var_assignment.push_back(b);	// 0 false 1 true
		}

		// Update Clause Cost 
		for (size_t c = 0; c < ClauseInfo.size(); c++) {
			ClauseCost[c] = 0;
			for (size_t l = 0; l < ClauseInfo[c].size(); l++) {
				// (0 pos ^ 1 true || 1 neg ^ 0 false) -> true
				if (ClauseInfo[c][l].varSign ^ var_assignment[ClauseInfo[c][l].varNumber - 1]) {
					ClauseCost[c]++;
				}
			}
			if (ClauseCost[c] == 0) {
				UCB.insert(c);
			}
		}

		// Start Flip
		for (size_t f = 0; f < MAX_FLIPS; f++) {
			if (UCB.size() == 0){
				issolved = true;
				for (int i = 0; i < numVars; i++) {
					answer[i] = var_assignment[i];
				}
				break;
			}
			
			int cnt = 0;
			int random_clause_idx = 0;
			int random_clause_sel = rand() % UCB.size();
			std::set<int>::iterator it;

			for( cnt = 0, it = UCB.begin(); it != UCB.end(); it++, cnt++ ){
				if( cnt == random_clause_sel ){
					random_clause_idx = *it;
				}
			}
			

			// Calculate break(x)
			int flip_var = 0;
			int break_min = 1e9;
			int make_max = 0;
			std::vector<int> var_candidates;
			for (size_t l = 0; l < ClauseInfo[random_clause_idx].size(); l++) {
				int break_cnt = 0;
				int lmake = 0;
				int var_num = ClauseInfo[random_clause_idx][l].varNumber;

				for (size_t c = 0; c < VarInClause[var_num - 1].size(); c++) {

					bool islittrue = VarInClause[var_num - 1][c].varSign ^ var_assignment[var_num - 1];	 // 0pos true , 1neg false
					int ccost = ClauseCost[VarInClause[var_num - 1][c].clsNumber - 1];
					// !islittrue
					if (islittrue && ccost == 1) {
						break_cnt++;
					}
					if (!islittrue) {
						if (ccost == 0) {
							lmake += W1;
						} else if (ccost == 1) {
							lmake += W2;
						}
						// lmake += ccost
					}
				}
				if (break_min >= break_cnt) {
					break_min = break_cnt;
					if (make_max <= lmake) {
						make_max = lmake;
						flip_var = var_num;
					}
				}
			}

			// choose among break 0s
			if (var_candidates.size() != 0) {
				flip_var = var_candidates[rand() % var_candidates.size()];
			}
			else {
				if (rand() % 1000 < RAND_FLIP) {
					flip_var = ClauseInfo[random_clause_idx][rand() % ClauseInfo[random_clause_idx].size()].varNumber;
				}
			}
			// flip
			for (int c = 0; c < VarInClause[flip_var - 1].size(); c++) {
				bool islittrue = VarInClause[flip_var - 1][c].varSign ^ var_assignment[flip_var - 1];	 // 0pos true , 1neg false
				int clsnum = VarInClause[flip_var - 1][c].clsNumber - 1;	// [0, ncls-1]
				if (islittrue) {	// true lit
					ClauseCost[clsnum]--;

					if (ClauseCost[clsnum] == 0) {	
						UCB.insert(clsnum);
					}
				}
				else {	// false lit
					if (ClauseCost[clsnum] == 0) {
						std::set<int>::iterator it = UCB.find(clsnum);
						if( it == UCB.end() ){
							std::cout << "Error. Cannot find clause " << clsnum << std::endl;
							exit(0);					
						}
						else {
							UCB.erase( it );
						}
					}
					ClauseCost[clsnum]++;
				}
			}
			var_assignment[flip_var - 1] = var_assignment[flip_var - 1] ^ true;
		} // end all flips
		if (issolved) break;
	} // end all tries

#ifdef DEBEG_R
	if ( UCB.size() != 0 ){
		std::cout << "\n\nWalkSAT could not find a solution." << std::endl;
	}
#endif
	std::chrono::steady_clock::time_point timeEnd = std::chrono::steady_clock::now();
	elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count();
}


void SAT_KCS::result() {
#ifdef DEBUG_R
	if (issolved) {
		bool totresult = true;
		for (int c = 0; c < numClauses; c++) {
			bool cls = false;
			for (int l = 0; l < ClauseInfo[c].size(); l++) {
				// true 1 pos 0, false 0 neg 1 -> true
				bool lit = answer[ClauseInfo[c][l].varNumber - 1] ^  ClauseInfo[c][l].varSign;
				cls |= lit;
			}
			totresult &= cls;
		}
		if (totresult) {
			std::cout << "WalkSAT found a solution. Verified.\n";
		}
	}
	else {
		std::cout << "WalkSAT could not find a solution.\n";
	}
	std::cout << "WalkSAT completed in: " << elapsedTime/1000000 << " seconds " << std::endl;
#endif
}