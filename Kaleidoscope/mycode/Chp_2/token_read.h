#include <string>

using namespace std;

//token 号码
enum Token {
  tok_eof = -1,

  // commands
  tok_def = -2, tok_extern = -3,

  // primary
  tok_identifier = -4, tok_number = -5
};

class token_read {
	public:
		string IdentifierStr;//用来存上一个字符串 
		double NumVal; //用来存上一个数字变量
		int LastChar;
		int Current_token;
		int gettok();	
		void getNextToken();

		token_read() {LastChar = ' ';}

};

int token_read::gettok() {
//{{{
	while (isspace(LastChar))
		LastChar = getchar();

  	if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
		IdentifierStr = LastChar;

		while (isalnum((LastChar = getchar())))
		  IdentifierStr += LastChar;
		//应该是在这里写keyword，
		if (IdentifierStr == "def") return tok_def;
		if (IdentifierStr == "extern") return tok_extern;
		return tok_identifier;
	}

	if (isdigit(LastChar) || LastChar == '.') {   // Number: [0-9.]+
		std::string NumStr;
		//取数字的
		do {
	  		NumStr += LastChar;
	  		LastChar = getchar();
		} while (isdigit(LastChar) || LastChar == '.');

		NumVal = strtod(NumStr.c_str(), 0);
		return tok_number;
	}

	if (LastChar == '#') {
	//取注释
		do LastChar = getchar();
		while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

		if (LastChar != EOF)
			return gettok();
	}

	if (LastChar == EOF)
		return tok_eof;
	
	int ThisChar = LastChar;
	LastChar = getchar();
	return ThisChar;
}
//}}}

//Token 读词器
void token_read::getNextToken() {
	Current_token = gettok();
}

