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


using namespace llvm;

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value *> NamedValues;



namespace {

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

class VariableExprAST : public ExprAST { //变量类（普通词）
//{{{
	string Name;
	public:
		  VariableExprAST(const std::string &Name) : Name(Name) {}
	  	Value *codegen() override;
};
//}}}

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST { //双目运算符
//{{{
  	char Op;
	std::unique_ptr<ExprAST> LHS, RHS;
	public:
  BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS) : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  		Value *codegen() override;
};
//}}}

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST { //表达式调用
//{{{ 
  	std::string Callee;
  	std::vector<std::unique_ptr<ExprAST>> Args;
	public:
	  CallExprAST(const std::string &Callee,
				  std::vector<std::unique_ptr<ExprAST>> Args)
		  : Callee(Callee), Args(std::move(Args)) {}
	  	Value *codegen() override;
};
//}}}

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
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

/// FunctionAST - This class represents a function definition itself.
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

}

std::unique_ptr<ExprAST> LogError(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}

unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
  LogError(Str);
  return nullptr;
}

/// LogError* - These are little helper functions for error handling.
Value *LogErrorV(const char *Str) {
  LogError(Str);
  return nullptr;
}



Value *NumberExprAST::codegen() {
//{{{
  return ConstantFP::get(TheContext, APFloat(Val));
}
//}}}

Value *VariableExprAST::codegen() {
//{{{
  // Look this variable up in the function.
  Value *V = NamedValues[Name];
  if (!V)
    return LogErrorV("Unknown variable name");
  return V;
}
//}}}

Value *BinaryExprAST::codegen() {
//{{{
  Value *L = LHS->codegen();
  Value *R = RHS->codegen();
  if (!L || !R)
    return nullptr;

  switch (Op) {
  case '+':
    return Builder.CreateFAdd(L, R, "addtmp");
  case '-':
    return Builder.CreateFSub(L, R, "subtmp");
  case '*':
    return Builder.CreateFMul(L, R, "multmp");
  case '<':
    L = Builder.CreateFCmpULT(L, R, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder.CreateUIToFP(L, Type::getDoubleTy(TheContext), "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}
//}}}

Value *CallExprAST::codegen() {
//{{{
  // Look up the name in the global module table.
  Function *CalleeF = TheModule->getFunction(Callee);
  if (!CalleeF)
    return LogErrorV("Unknown function referenced");

  // If argument mismatch error.
  if (CalleeF->arg_size() != Args.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    ArgsV.push_back(Args[i]->codegen());
    if (!ArgsV.back())
      return nullptr;
  }

  return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}
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





