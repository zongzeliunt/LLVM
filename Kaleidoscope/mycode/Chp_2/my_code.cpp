#include <cstdio>
#include <cstdlib>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include "parser.h"


using namespace std;







int main () {
	
	static token_read token_read_hl;
	static parser parser_hl(token_read_hl);

	parser_hl.MainLoop();
}
