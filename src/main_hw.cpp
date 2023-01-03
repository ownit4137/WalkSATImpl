#include "sat.h"

int main(int argc, char* argv[]){
	srand(time(0));
	std::string filepath(argv[1]);
	std::cout << "\nfilename: " << filepath << "\n";
	static WSAT_HW inst(filepath);
	inst.solve();
	inst.result();

	return 0;
}