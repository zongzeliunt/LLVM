#include <iostream>
#include "token_read.h"
#include "AST.h"

class parser {
	public:
		//成员
		token_read token_read_hl;
		std::map<char, int> BinopPrecedence;

		parser (token_read token_read_p); 
		int GetTokPrecedence(int Current_token);
		void MainLoop();

		//顶层：
		std::unique_ptr<FunctionAST> ParseDefinition();
		void HandleDefinition();

		std::unique_ptr<PrototypeAST> ParseExtern();
		void HandleExtern(); 

		std::unique_ptr<FunctionAST> ParseTopLevelExpr();
		void HandleTopLevelExpression();

		//内部：	
		std::unique_ptr<ExprAST> ParseIdentifierExpr();

		std::unique_ptr<ExprAST> ParseNumberExpr();

		std::unique_ptr<ExprAST> ParseParenExpr();

		std::unique_ptr<ExprAST> ParsePrimary();

		std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS); 

		std::unique_ptr<ExprAST> ParseExpression();

		std::unique_ptr<PrototypeAST> ParsePrototype();
	
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
std::unique_ptr<FunctionAST> parser::ParseDefinition() {
  token_read_hl.getNextToken();  // eat def.
  auto Proto = ParsePrototype();
  if (!Proto)
    return nullptr;

  if (auto E = ParseExpression())
    return make_unique<FunctionAST>(std::move(Proto), std::move(E));
  return nullptr;
}

void parser::HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
    if (auto *FnIR = FnAST->codegen()) {
      fprintf(stderr, "Read function definition:");
      FnIR->print(errs());
      fprintf(stderr, "\n");
    }
  } else {
    // Skip token for error recovery.
    token_read_hl.getNextToken();
  }
}
//}}}

/// external ::= 'extern' prototype
//{{{
std::unique_ptr<PrototypeAST> parser::ParseExtern() {
  token_read_hl.getNextToken();  // eat extern.
  return ParsePrototype();
}

void parser::HandleExtern() {
  if (auto ProtoAST = ParseExtern()) {
    if (auto *FnIR = ProtoAST->codegen()) {
      fprintf(stderr, "Read extern: ");
      FnIR->print(errs());
      fprintf(stderr, "\n");
    }
  } else {
    // Skip token for error recovery.
    token_read_hl.getNextToken();
  }
}
//}}}

//普通表达式 expression 解析器
//{{{
std::unique_ptr<FunctionAST> parser::ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Make an anonymous proto.
    auto Proto = make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
    return make_unique<FunctionAST>(std::move(Proto), std::move(E));
  }
  return nullptr;
}

void parser::HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto FnAST = ParseTopLevelExpr()) {
	//FnAST 是一个FunctionAST类型
    if (auto *FnIR = FnAST->codegen()) {
      fprintf(stderr, "Read top-level expression:");
      FnIR->print(errs());
      fprintf(stderr, "\n");
    }
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
std::unique_ptr<ExprAST> parser::ParseIdentifierExpr() {
  std::string IdName = token_read_hl.IdentifierStr;

  token_read_hl.getNextToken();  // eat identifier.

  if (token_read_hl.Current_token != '(') // Simple variable ref.
    return make_unique<VariableExprAST>(IdName);

  // Call.
  token_read_hl.getNextToken();  // eat (
  std::vector<std::unique_ptr<ExprAST>> Args;
  if (token_read_hl.Current_token != ')') {
    while (1) {
      auto Arg = ParseExpression();
      if (!Arg) return 0;
      Args.push_back(std::move(Arg));

      if (token_read_hl.Current_token == ')') break;

      if (token_read_hl.Current_token != ',')
        return LogError("Expected ')' or ',' in argument list");
      token_read_hl.getNextToken();
    }
  }

  // Eat the ')'.
  token_read_hl.getNextToken();

  return make_unique<CallExprAST>(IdName, std::move(Args));
}
//}}}

/// numberexpr ::= number
//{{{
std::unique_ptr<ExprAST> parser::ParseNumberExpr() {
  auto Result = make_unique<NumberExprAST>(token_read_hl.NumVal);
  token_read_hl.getNextToken(); // consume the number
  return std::move(Result);
}
//}}}

/// parenexpr ::= '(' expression ')'
//{{{
std::unique_ptr<ExprAST> parser::ParseParenExpr() {
  token_read_hl.getNextToken();  // eat (.
  auto V = ParseExpression();
  if (!V) return 0;

  if (token_read_hl.Current_token != ')')
    return LogError("expected ')'");
  token_read_hl.getNextToken();  // eat ).
  return V;
}
//}}}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
//{{{
std::unique_ptr<ExprAST> parser::ParsePrimary() {
  switch (token_read_hl.Current_token) {
  default: return LogError("unknown token when expecting an expression");
  case tok_identifier: return ParseIdentifierExpr();
  case tok_number:     return ParseNumberExpr();
  case '(':            return ParseParenExpr();
  }
}
//}}}

/// binoprhs
///   ::= ('+' primary)*
//{{{
std::unique_ptr<ExprAST> parser::ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) {
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
    auto RHS = ParsePrimary();
    if (!RHS) return 0;

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = GetTokPrecedence(token_read_hl.Current_token);
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec+1, std::move(RHS));
      if (RHS == 0) return 0;
    }

    // Merge LHS/RHS.
    LHS = make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
  }
}
//}}}

/// expression
///   ::= primary binoprhs
//{{{
std::unique_ptr<ExprAST> parser::ParseExpression() {
  auto LHS = ParsePrimary();
  if (!LHS) return 0;

  return ParseBinOpRHS(0, std::move(LHS));
}
//}}}

/// prototype
///   ::= id '(' id* ')'
//{{{
std::unique_ptr<PrototypeAST> parser::ParsePrototype() {
  if (token_read_hl.Current_token != tok_identifier)
    return LogErrorP("Expected function name in prototype");

  std::string FnName = token_read_hl.IdentifierStr;
  token_read_hl.getNextToken();

  if (token_read_hl.Current_token != '(')
    return LogErrorP("Expected '(' in prototype");

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
    return LogErrorP("Expected ')' in prototype");

  // success.
  token_read_hl.getNextToken();  // eat ')'.

  return make_unique<PrototypeAST>(FnName, ArgNames);
}
//}}}







