#include <iostream>
#include "token_read.h"
#include "AST.h"


using namespace std;

class parser {
	public:
		//成员
		token_read token_read_hl;
		std::map<char, int> BinopPrecedence;

		parser (token_read token_read_p); 
		int GetTokPrecedence(int Current_token);
		void MainLoop();

		//顶层：
		FunctionAST *ParseDefinition();
		void HandleDefinition();

		PrototypeAST *ParseExtern();
		void HandleExtern(); 

		FunctionAST *ParseTopLevelExpr();
		void HandleTopLevelExpression();

		//内部：	
		ExprAST *ParseIdentifierExpr();

		ExprAST *ParseNumberExpr();

		ExprAST *ParseParenExpr();

		ExprAST *ParsePrimary();

		ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS); 

		ExprAST *ParseExpression();

		PrototypeAST *ParsePrototype();
	
};

parser::parser (token_read token_read_p) {
	token_read_hl = token_read_p; 
	BinopPrecedence['<'] = 10;
	BinopPrecedence['+'] = 20;
	BinopPrecedence['-'] = 20;
	BinopPrecedence['*'] = 40;
}

int parser::GetTokPrecedence(int Current_token) {
//{{{
  if (!isascii(Current_token))
    return -1;

  int TokPrec = BinopPrecedence[Current_token];
  if (TokPrec <= 0) return -1;
  return TokPrec;
}
//}}}

void parser::MainLoop() {
//{{{
	//在这里先吃第一个字
	cout<<"ready>";
	token_read_hl.getNextToken();

	while (1) {
		cout<<"ready>";
    	//fprintf(stderr, "ready> ");
		switch (token_read_hl.Current_token) {
			case tok_eof: 
				return;
			case ';':
				token_read_hl.getNextToken(); break;
			case tok_def: 
				HandleDefinition(); break;
			case tok_extern: 
				HandleExtern(); break;
			default:
				HandleTopLevelExpression(); break;
		}
	}
}
//}}}



//顶层 parser (解析器)

//定义式 def 解析器
//{{{
FunctionAST *parser::ParseDefinition() {
  token_read_hl.getNextToken();  // eat def.
  PrototypeAST *Proto = ParsePrototype();
  if (Proto == 0) return 0;

  if (ExprAST *E = ParseExpression())
    return new FunctionAST(Proto, E);
  return 0;
}

void parser::HandleDefinition() {
  if (ParseDefinition()) {
    cout<<"Parsed a function definition.\n";
  } else {
    // Skip token for error recovery.
    token_read_hl.getNextToken();
  }
}
//}}}

/// external ::= 'extern' prototype
//{{{
PrototypeAST *parser::ParseExtern() {
  token_read_hl.getNextToken();  // eat extern.
  return ParsePrototype();
}

void parser::HandleExtern() {
  if (ParseExtern()) {
    cout<<"Parsed an extern\n";
  } else {
    // Skip token for error recovery.
    token_read_hl.getNextToken();
  }
}
//}}}

//普通表达式 expression 解析器
//{{{
FunctionAST *parser::ParseTopLevelExpr() {
  if (ExprAST *E = ParseExpression()) {
    // Make an anonymous proto.
    PrototypeAST *Proto = new PrototypeAST("", std::vector<std::string>());
    return new FunctionAST(Proto, E);
  }
  return 0;
}

void parser::HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (ParseTopLevelExpr()) {
    //fprintf(stderr, "Parsed a top-level expr\n");
  	cout<<"Parsed a top-level expr"<<endl;
  } else {
    // Skip token for error recovery.
    token_read_hl.getNextToken();
  }
}
//}}}


/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
//{{{
ExprAST *parser::ParseIdentifierExpr() {
  std::string IdName = token_read_hl.IdentifierStr;

  token_read_hl.getNextToken();  // eat identifier.

  if (token_read_hl.Current_token != '(') // Simple variable ref.
    return new VariableExprAST(IdName);

  // Call.
  token_read_hl.getNextToken();  // eat (
  std::vector<ExprAST*> Args;
  if (token_read_hl.Current_token != ')') {
    while (1) {
      ExprAST *Arg = ParseExpression();
      if (!Arg) return 0;
      Args.push_back(Arg);

      if (token_read_hl.Current_token == ')') break;

      if (token_read_hl.Current_token != ',')
        return Error("Expected ')' or ',' in argument list");
      token_read_hl.getNextToken();
    }
  }

  // Eat the ')'.
  token_read_hl.getNextToken();

  return new CallExprAST(IdName, Args);
}
//}}}

/// numberexpr ::= number
//{{{
ExprAST *parser::ParseNumberExpr() {
  ExprAST *Result = new NumberExprAST(token_read_hl.NumVal);
  token_read_hl.getNextToken(); // consume the number
  return Result;
}
//}}}

/// parenexpr ::= '(' expression ')'
//{{{
ExprAST *parser::ParseParenExpr() {
  token_read_hl.getNextToken();  // eat (.
  ExprAST *V = ParseExpression();
  if (!V) return 0;

  if (token_read_hl.Current_token != ')')
    return Error("expected ')'");
  token_read_hl.getNextToken();  // eat ).
  return V;
}
//}}}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
//{{{
ExprAST *parser::ParsePrimary() {
  switch (token_read_hl.Current_token) {
  default: return Error("unknown token when expecting an expression");
  case tok_identifier: return ParseIdentifierExpr();
  case tok_number:     return ParseNumberExpr();
  case '(':            return ParseParenExpr();
  }
}
//}}}

/// binoprhs
///   ::= ('+' primary)*
//{{{
ExprAST *parser::ParseBinOpRHS(int ExprPrec, ExprAST *LHS) {
  // If this is a binop, find its precedence.
  while (1) {
    int TokPrec = GetTokPrecedence(token_read_hl.Current_token);

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec)
      return LHS;

    // Okay, we know this is a binop.
    int BinOp = token_read_hl.Current_token;
    token_read_hl.getNextToken();  // eat binop

    // Parse the primary expression after the binary operator.
    ExprAST *RHS = ParsePrimary();
    if (!RHS) return 0;

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = GetTokPrecedence(token_read_hl.Current_token);
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec+1, RHS);
      if (RHS == 0) return 0;
    }

    // Merge LHS/RHS.
    LHS = new BinaryExprAST(BinOp, LHS, RHS);
  }
}
//}}}

/// expression
///   ::= primary binoprhs
//{{{
ExprAST *parser::ParseExpression() {
  ExprAST *LHS = ParsePrimary();
  if (!LHS) return 0;

  return ParseBinOpRHS(0, LHS);
}
//}}}

/// prototype
///   ::= id '(' id* ')'
//{{{
PrototypeAST *parser::ParsePrototype() {
  if (token_read_hl.Current_token != tok_identifier)
    return ErrorP("Expected function name in prototype");

  std::string FnName = token_read_hl.IdentifierStr;
  token_read_hl.getNextToken();

  if (token_read_hl.Current_token != '(')
    return ErrorP("Expected '(' in prototype");

  std::vector<std::string> ArgNames;
	while (1) {
		token_read_hl.getNextToken();
		if (token_read_hl.Current_token == tok_identifier) {
			ArgNames.push_back(token_read_hl.IdentifierStr);
		} else {
			break;
		}
	}


  if (token_read_hl.Current_token != ')')
    return ErrorP("Expected ')' in prototype");

  // success.
  token_read_hl.getNextToken();  // eat ')'.

  return new PrototypeAST(FnName, ArgNames);
}
//}}}







