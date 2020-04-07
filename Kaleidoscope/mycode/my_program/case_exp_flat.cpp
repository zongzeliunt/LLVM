/*
	这个程序是将原程序完全扁平化，不存在任何类，只突出LLVM所包含的API调用


*/
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


int main () {
  	
	TheModule = make_unique<Module>("my cool jit", TheContext);
	std::cout<<"hello world"<<std::endl;

	double LHS = 5.0;
	double RHS = 3.0;
	char BinOp = '+';
	
	//proto
	std::string Proto_Name = "__anon_expr";
	std::vector<std::string> Proto_Args;
	
	//	BinaryExprAST E 其实没必要声明

	//以下内容来自FunctionAST::codegen()
		//FunctionAST 其实有两个member，一个proto一个body，body就是BinaryExprAST E


	Function *TheFunction = TheModule->getFunction(Proto_Name);

  	if (!TheFunction) {
	//来自 PrototypeAST::codegen():
  		std::vector<Type *> Doubles(Proto_Args.size(), Type::getDoubleTy(TheContext));
		//FunctionType 应该是来自Function.h
 	 	FunctionType *FT = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

       	TheFunction = Function::Create(FT, Function::ExternalLinkage, Proto_Name, TheModule.get());

		unsigned Idx = 0;
		for (auto &Arg : TheFunction->args())
			Arg.setName(Proto_Args[Idx++]);
	}

	BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
	Builder.SetInsertPoint(BB);

  	// Record the function arguments in the NamedValues map.
  	NamedValues.clear();
  	for (auto &Arg : TheFunction->args())
  	  	NamedValues[std::string(Arg.getName())] = &Arg;

	//来自BinaryExprAST E的codegen
	Value *L =	ConstantFP::get(TheContext, APFloat(LHS));
	Value *R =	ConstantFP::get(TheContext, APFloat(RHS));

   	Value *RetVal = Builder.CreateFAdd(L, R, "addtmp");

    Builder.CreateRet(RetVal);
    
	verifyFunction(*TheFunction);


    TheFunction->print(errs());






	return 0;
}
