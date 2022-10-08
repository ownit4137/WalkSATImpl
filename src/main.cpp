#include "sat.h"

class SATInstance;

int main(int argc, char* argv[]){

	std::string path = std::string(argv[1]);
	SAT_KCS inst(path);
	// inst.solve(std::stoi(std::string(argv[2])));
	// inst.WalkSat(); //ykchoi
	inst.PrintClauseList();
	inst.PrintLiteralList();
	return 0;
}
