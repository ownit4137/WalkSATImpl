#include "sat.h"

int main(int argc, char* argv[]){
	srand(time(0));
	std::string filepath(argv[1]);
	std::cout << "\nfilename: " << filepath << "\n";
	SAT_KCS inst(filepath);
	inst.solve();
	inst.result();

	return 0;
}