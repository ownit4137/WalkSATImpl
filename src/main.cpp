#include "sat.h"

int main(int argc, char* argv[]){

	std::string path = std::string(argv[1]);
	std::cout << "\nfilename: " << path << "\n";
	SAT_KCS inst(path);
	
	inst.solve();
	inst.result();

	return 0;
}