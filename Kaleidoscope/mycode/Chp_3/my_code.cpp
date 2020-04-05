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
	
	//TheModule 和 TheContext是AST.h里声明的
  	TheModule = make_unique<Module>("my cool jit", TheContext);

	parser_hl.MainLoop();
  	TheModule->print(errs(), nullptr);
}
