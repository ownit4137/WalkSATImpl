#include "sat.h"

int main(int argc, char* argv[]){

	std::string path = std::string(argv[1]);
	std::cout << path;
	SAT_KCS inst(path);
	// inst.solve(std::stoi(std::string(argv[2])));
	// inst.WalkSat(); //ykchoi
	// inst.PrintVarInClause();
	// inst.PrintClauseInfo();
	// inst.solve();
	inst.result();
	return 0;
}
