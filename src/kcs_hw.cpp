#include "sat.h"


void WSAT_HW::ucbInsertHash(int clnum) {		// 1 ~ #cls
	int i = clnum % UCBSIZE;
	while (true) {
		if (UCBArr[i] == 0) {
			UCBArr[i] = clnum;
			break;
		}
		collision++;
		i = (i + 1) % UCBSIZE;
	}
}

void WSAT_HW::ucbEraseHash(int clnum) {
	int i = clnum % UCBSIZE;
	while (true) {
		if (UCBArr[i] == clnum) {
			UCBArr[i] = 0;
			break;
		}
		collision++;
		i = (i + 1) % UCBSIZE;
	}
}

void WSAT_HW::ucbInsertArr(int clnum) {
	// std::cout << clnum << "\n\n";
	for (int i = 0; i < ucblast; i++) {
		if (UCBArr[i] == clnum) {
			return;
		}
	}
	UCBArr[ucblast] = clnum;
	ucblast++;
}

void WSAT_HW::ucbEraseArr(int clnum) {
	// UCBPosArr
	for (int i = 0; i < ucblast; i++) {
		if (UCBArr[i] == clnum) {
			UCBArr[i] = UCBArr[ucblast - 1];
			ucblast--;
			break;
		}
	}
}


WSAT_HW::WSAT_HW(std::string path) {
	std::ifstream fileDIMACS(path);

	int maxk = 0;

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

		// std::cout << numVars << "," << numClauses << "\n";

		VarInClause.resize(numVars, std::vector<cls>(0));

		int nextLit;
		int clauseCounted = 1;

		char p;
		int nv = 0;	// #var in a clause
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
			if ((p >= '0' && p <= '9') || p == '-') {
				// std::cout << "cls " << clauseCounted << " || ";
				while (true) {
					fileDIMACS >> nextLit;
					if (nextLit == 0){	// end of line
						// lit tl;
						// tl.varSign = true;
						// tl.varNumber = 0;
						// ClausesVec[clauseCounted - 1][nv] = tl;
						
						if (nv > maxk) {
							maxk = nv;
						}
						nv = 0;
						break;
					}
					// std::cout << nextLit << " ";

					lit tl;
					cls tc;
					tl.varSign = nextLit < 0;	// - true 1 / + false 0
					tl.varNumber = tl.varSign ? -nextLit : nextLit;	// +- 1 ~ nvar
					tc.varSign = nextLit < 0;
					tc.clsNumber = clauseCounted;					// +- 1 ~ ncls

					ClausesVec[clauseCounted - 1][nv] = tl;
					VarInClause[tl.varNumber - 1].push_back(tc);
					nv++;
				}
				// std::cout << "\n";
				clauseCounted++;
			}
		}
		
		if (clauseCounted - 1 != numClauses) {
			std::cout << "#clauses error || written: " << numClauses << " counted: " << clauseCounted - 1 << "\n";
		}

	} else {
		std::cerr << "Cannot open file: " << path << "\n";
	}

	std::cout << "maxk: " << maxk << "\n";
	
	// varinclause -> varslocvec
	/*
	int start = 0;
	int end = 0;
	int i = 0;
	for (int v = 0; v < numVars; v++) {
		start = i;
		for (int j = 0; j < VarInClause[v].size(); j++) {
			VarsLocVec[i] = VarInClause[v][j];
			i++;
		}
		end = i;
		AddTransT[v] = start * multiplier + end;
	}
	VLVend = end;
	*/
}

void WSAT_HW::info() {
	bool issaved = true;
	for (int v = 0; v < numVars; v++) {
		int st = AddTransT[v] / multiplier;
		int ed = AddTransT[v] % multiplier;
		for (int i = st; i < ed; i++) {
			if (VarsLocVec[i] != VarInClause[v][i - st]) {
				issaved = false;
				break;
			}
		}
	}
	if (!issaved) {
		std::cout << "wrong initialization\n";
	}
}

void WSAT_HW::reset() {
	// VATArr initialize, ClausesCost, UCB reset
	for (int i = 0; i < numVars; i++) {
		VATArr[i] = rand() % 2 == 0 ? true : false;
		// VATArr[i] = i % 2 == 0 ? true : false;
	}

	// for (int i = 0; i < numVars; i++) {
	// 	std::cout << VATArr[i] << " ";
	// }
	// std::cout << "\n";

	for (int i = 0; i < numClauses; i++) {
		ClausesCost[i] = 0;
		UCBArr[i] = 0;
	}

	ucblast = 0;
}

void WSAT_HW::updateCost() {
	// Update Clause Cost [Phase1]
	for (int c = 0; c < numClauses; c++) {
		// std::cout << "clsnum: " << c + 1 << " || ";
		int cost = 0;
		for (int l = 0; l < K; l++) {	// static K
			// (0 pos ^ 1 true || 1 neg ^ 0 false) -> true
			
			if (ClausesVec[c][l].varSign ^ VATArr[ClausesVec[c][l].varNumber - 1]) {
				// std::cout << ClausesVec[c][l].varNumber << ": " << VATArr[ClausesVec[c][l].varNumber - 1] << " | ";
				cost++;
			}
		}
	
		if (cost == 0) {
			ucbInsertArr(c);
		}
		ClausesCost[c] = cost;
		// std::cout << " cost: " << ClausesCost[c];
		// std::cout << "\n";
	}
}

void WSAT_HW::solve() {
	std::chrono::steady_clock::time_point timeStart = std::chrono::steady_clock::now();
	

	// [[  TRY  ]]
	for (size_t t = 0; t < MAX_TRIES; t++) {
		std::cout << "TRY-"<< t + 1 << "\n";

		// random reset or heuristic reset
		reset();
		updateCost();
		
		// [[  FLIP  ]]
		for (size_t f = 0; f < MAX_FLIPS; f++) {
			// std::cout << ucblast << "\n";

			// finish cond
			if (ucblast == 0){
				issolved = true;
				maxflip = f;
				for (int i = 0; i < numVars; i++) {
					answer[i] = VATArr[i];
				}
				break;
			}

			
			
			if (f % (MAX_FLIPS/PRINT_FLIP_TIMES) == 0) {
				std::cout << "FLIP-"<< f + 1 << " UCB size: " << ucblast <<  "\n";
				// for (int i = 0; i < numVars; i++) {
				// 	std::cout << VATArr[i] << " ";
				// }
				// std::cout << "\n";
				if (ucblast < 20) {
					printUCB();
				}
			}
			
			// random ucb selection cls +-[0, ncls-1]
			int ucidx = UCBArr[rand() % ucblast];
			
			// std::cout << "uc idx: " << ucidx << " selected\n";

			// [ choose var in var candidates ]
			int flip_var = 0;
			int break_min = 1e9;
			int make_max = 0;
			// int clength = 0;
			
			for (size_t l = 0; l < K; l++) {
				int break_cnt = 0;
				int make_cnt = 0;
				int var_num = ClausesVec[ucidx][l].varNumber;
				// if (var_num == 0) {
				// 	clength = l;
				// 	break;
				// }
				
				/*
				int st = AddTransT[var_num - 1] / multiplier;
				int ed = AddTransT[var_num - 1] % multiplier;
				for (int c = st; c < ed; c++) {
					bool islittrue = VarsLocVec[c].varSign ^ VATArr[var_num - 1];	 // 0pos true , 1neg false
					int ccost = ClausesCost[VarsLocVec[c].clsNumber - 1];
					
					if (islittrue && ccost == 1) {	// break
						break_cnt++;
					}
					if (!islittrue && ccost == 0) {	// make
						make_cnt++;
					}
				}
				*/

				// std::cout << "var " << var_num << " is in cl ";
				for (size_t c = 0; c < VarInClause[var_num - 1].size(); c++) {
					// std::cout << VarInClause[var_num - 1][c].clsNumber << " ";

					bool islittrue = VarInClause[var_num - 1][c].varSign ^ VATArr[var_num - 1];	 // 0pos true , 1neg false
					int ccost = ClausesCost[VarInClause[var_num - 1][c].clsNumber - 1];

					if (islittrue) {
						if (ccost == 1) {
							// std::cout << "[b" << VarInClause[var_num - 1][c].clsNumber << "] ";
							break_cnt++;
						}
					}
					if (!islittrue) {
						if (ccost == 0) {
							// std::cout << "[m" << VarInClause[var_num - 1][c].clsNumber << "] ";
							make_cnt++;
						}
					}
				}
				// std::cout << "\n";
				
				if (break_min > break_cnt) {
					break_min = break_cnt;
					make_max = make_cnt;
					flip_var = var_num;
				}
				else if (break_min == break_cnt) {
					if (make_max < make_cnt) {
						break_min = break_cnt;
						make_max = make_cnt;
						flip_var = var_num;
					}
				}
			}

			if (rand() % 1000 < RAND_FLIP) {
				flip_var = ClausesVec[ucidx][rand() % K].varNumber;		// +-[1, nvar]
				//std::cout << "Var " << flip_var << " random\n";
			} 
			// else {
			// 	if (f % (MAX_FLIPS/PRINT_FLIP_TIMES) == 0) {
			// 	std::cout << "Var " << flip_var << " in Clause " << ucidx + 1 << " break " << break_min << " make " << make_max << "\n";
			// 	}
			// }

			/*
			// [ actual flip ]
			int st = AddTransT[flip_var - 1] / multiplier;
			int ed = AddTransT[flip_var - 1] % multiplier;
			for (int c = st; c < ed; c++) {
				bool islittrue = VarsLocVec[c].varSign ^ VATArr[flip_var - 1];	 // 0pos true , 1neg false
				int clsnum = VarsLocVec[c].clsNumber - 1;	// +- [0, ncls)
				int update = 0;

				if (islittrue) {
					update = -1;
					if (ClausesCost[clsnum] == 1) {
						ucbInsertArr(clsnum);
					}
				}
				else {
					update = 1;
					if (ClausesCost[clsnum] == 0) {
						ucbEraseArr(clsnum);
					}
				}
				ClausesCost[clsnum] += update;
			}
			VATArr[flip_var - 1] = VATArr[flip_var - 1] ^ true;
			*/

			// std::cout << "UCB update || ";

			for (size_t c = 0; c < VarInClause[flip_var - 1].size(); c++) {

				bool islittrue = VarInClause[flip_var - 1][c].varSign ^ VATArr[flip_var - 1];	 // 0pos true , 1neg false
				int clsnum = VarInClause[flip_var - 1][c].clsNumber - 1;	// [0, ncls-1]
				int update = 0;
				
				if (islittrue) {
					update = -1;
					if (ClausesCost[clsnum] == 1) {
						// std::cout << "+" << clsnum + 1 << ":" << ClausesCost[clsnum] << " ";
						ucbInsertArr(clsnum);
					}
				}
				else {
					update = 1;
					if (ClausesCost[clsnum] == 0) {
						// std::cout << "-" << clsnum + 1 << ":" << ClausesCost[clsnum] << " ";
						ucbEraseArr(clsnum);
					}
				}
				ClausesCost[clsnum] += update;
			}

			// std::cout << "\n";
			VATArr[flip_var - 1] = VATArr[flip_var - 1] ^ true;


		
		} // end all flips
		if (issolved) break;
	} // end all tries
	
	std::chrono::steady_clock::time_point timeEnd = std::chrono::steady_clock::now();
	elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count();
}



void WSAT_HW::result() {
	if (issolved) {
		bool totresult = true;
		for (int c = 0; c < numClauses; c++) {
			bool cls = false;
			for (int l = 0; l < K; l++) {
				// true 1 pos 0, false 0 neg 1 -> true
				bool lit = answer[ClausesVec[c][l].varNumber - 1] ^  ClausesVec[c][l].varSign;
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
	std::cout << "WalkSAT completed in: " << elapsedTime/1000000 << " seconds | flip: " << maxflip << std::endl;
}
