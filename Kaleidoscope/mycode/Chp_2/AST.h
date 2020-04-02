
using namespace std;

class ExprAST { //基类
	public:
  		virtual ~ExprAST() {}
};

class NumberExprAST : public ExprAST {//数字类

	double Val;
	public:
		NumberExprAST(double val) : Val(val) {}
};

class VariableExprAST : public ExprAST { //变量类（普通词）
	string Name;
	public:
		VariableExprAST(string &name) : Name(name) {}
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST { //双目运算符
  	char Op;
  	ExprAST *LHS, *RHS;
	public:
  		BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs)
    		: Op(op), LHS(lhs), RHS(rhs) {}
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST { //表达式调用 
  	std::string Callee;
  	std::vector<ExprAST*> Args;
	public:
  		CallExprAST(const std::string &callee, std::vector<ExprAST*> &args)
    		: Callee(callee), Args(args) {}
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
//{{{
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
public:
  PrototypeAST(const std::string &name, const std::vector<std::string> &args)
    : Name(name), Args(args) {}

};
//}}}

/// FunctionAST - This class represents a function definition itself.
//{{{
class FunctionAST {
  	PrototypeAST *Proto;
  	ExprAST *Body;
	public:
  		FunctionAST(PrototypeAST *proto, ExprAST *body)
    		: Proto(proto), Body(body) {}

};
//}}}


/// Error* - These are little helper functions for error handling.
ExprAST *Error(const char *Str) { 
	fprintf(stderr, "Error: %s\n", Str);
	return 0;
}

PrototypeAST *ErrorP(const char *Str) { 
	Error(Str); 
	return 0; 
}

FunctionAST *ErrorF(const char *Str) { 
	Error(Str); 
	return 0; 
}
