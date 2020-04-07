#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

using namespace llvm;

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value *> NamedValues;





/*
===============NODE=====================
NUMBER
	HandleTopLevelExpression
		1. FnAST 是一个FunctionAST类型, ParseTopLevelExpr生成出来的
		ParseTopLevelExpr
			E是一个std::unique_ptr<BinaryExprAST> 类型的指针！ ParseExpression 生成出来
			BinaryExprAST 里面的两个member LHS RHS 都是std::unique_ptr<NumberExprAST>的指针！



			ParseExpression
				ParsePrimary
					ParsePrimary 这个函数是输出一个 NumberExprAST 类型的数据的地方，因为有数字它就执行ParseNumberExpr，返回一个NumberExprAST类型数据
	
				ParseBinOpRHS
					ParseBinOpRHS 才是真正的处理表达式的地方，输出是一个处理完的表达式
		2. FunctionAST 的codegen
		FnIR = FnAST->codegen()

*/



class ExprAST { //基类
	public:
  		virtual ~ExprAST() {}
  		virtual Value *codegen() = 0;
};

class NumberExprAST : public ExprAST {//数字类
//{{{
	double Val;
	public:
		NumberExprAST(double val) : Val(val) {}
	  	Value *codegen() override;
};
//}}}

Value *NumberExprAST::codegen() {
//{{{
  return ConstantFP::get(TheContext, APFloat(Val));
}
//}}}

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST { //双目运算符
//{{{
  	char Op;
	std::unique_ptr<NumberExprAST> LHS, RHS;
	//NumberExprAST LHS, RHS;
	public:
	BinaryExprAST(char Op, std::unique_ptr<NumberExprAST> LHS,
                std::unique_ptr<NumberExprAST> RHS) : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  		Value *codegen() override;
  	
	/*
	BinaryExprAST(char Op, NumberExprAST LHS, NumberExprAST RHS) 
		: Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  	*/
};
//}}}

Value *BinaryExprAST::codegen() {
//{{{
  	/*
	Value *L = LHS.codegen();
  	Value *R = RHS.codegen();
  	*/
	Value *L = LHS->codegen();
  	Value *R = RHS->codegen();
  
	if (!L || !R)
    return nullptr;

  switch (Op) {
  case '+': 
	{
		std::cout<<"add"<<std::endl;
    	return Builder.CreateFAdd(L, R, "addtmp");
	}
  case '-':
    return Builder.CreateFSub(L, R, "subtmp");
  case '*':
    return Builder.CreateFMul(L, R, "multmp");
  case '<':
    L = Builder.CreateFCmpULT(L, R, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder.CreateUIToFP(L, Type::getDoubleTy(TheContext), "booltmp");
  default:
    return nullptr;
  }
}
//}}}

class PrototypeAST {
//{{{
  	std::string Name;
  	std::vector<std::string> Args;
	public:
  		PrototypeAST(const std::string &name, const std::vector<std::string> &args) : Name(name), Args(args) {}
		Function *codegen();
		const std::string &getName() const { return Name; }

};
//}}}

Function *PrototypeAST::codegen() {
//{{{
  // Make the function type:  double(double,double) etc.
  std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(TheContext));
  FunctionType *FT =
      FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

  Function *F =
      Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(Args[Idx++]);

  return F;
}
//}}}

class FunctionAST {
//{{{
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;
	public:
	  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
				  std::unique_ptr<ExprAST> Body)
		  : Proto(std::move(Proto)), Body(std::move(Body)) {}
		Function *codegen();

};
//}}}

Function *FunctionAST::codegen() {
//{{{
  // First, check for an existing function from a previous 'extern' declaration.
  
	Function *TheFunction = TheModule->getFunction(Proto->getName());

  if (!TheFunction)
    TheFunction = Proto->codegen();

  if (!TheFunction)
    return nullptr;

  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
  Builder.SetInsertPoint(BB);

  // Record the function arguments in the NamedValues map.
  NamedValues.clear();
  for (auto &Arg : TheFunction->args())
    NamedValues[std::string(Arg.getName())] = &Arg;

  if (Value *RetVal = Body->codegen()) {
    // Finish off the function.
    Builder.CreateRet(RetVal);

    // Validate the generated code, checking for consistency.
    verifyFunction(*TheFunction);

    return TheFunction;
  }

  // Error reading body, remove function.
  TheFunction->eraseFromParent();
  return nullptr;
}
//}}}







int main () {
  	
	TheModule = make_unique<Module>("my cool jit", TheContext);
	std::cout<<"hello world"<<std::endl;
	std::unique_ptr<NumberExprAST> LHS = make_unique<NumberExprAST>(2.0);
	std::unique_ptr<NumberExprAST> RHS = make_unique<NumberExprAST>(3.0);
	char BinOp = '+';

	std::unique_ptr<BinaryExprAST> E = make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));

    std::unique_ptr<PrototypeAST> Proto = make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
    //auto Proto = make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());

	std::unique_ptr<FunctionAST> FnAST = make_unique<FunctionAST>(std::move(Proto), std::move(E));
	
	Function *FnIR = FnAST->codegen();
    
	fprintf(stderr, "Read top-level expression:");
    FnIR->print(errs());
    fprintf(stderr, "\n");

  	TheModule->print(errs(), nullptr);
	return 0;
}
