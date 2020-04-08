/*
	这个程序是将原程序完全扁平化，不存在任何类，只突出LLVM所包含的API调用


*/
#include "../../include/KaleidoscopeJIT.h"
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
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
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
using namespace llvm::orc;

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value *> NamedValues;
static std::unique_ptr<KaleidoscopeJIT> TheJIT;


int main () {
  	
  	InitializeNativeTarget();
  	InitializeNativeTargetAsmPrinter();
  	InitializeNativeTargetAsmParser();


	TheModule = make_unique<Module>("my cool jit", TheContext);
	std::cout<<"hello world"<<std::endl;

	double LHS = 5.0;
	double RHS = 3.0;
	char BinOp = '+';
	
  	TheJIT = make_unique<KaleidoscopeJIT>();

	//proto 声明
	std::string Proto_Name = "__anon_expr";
	std::vector<std::string> Proto_Args;
	
	//	BinaryExprAST E 其实没必要声明

	//以下内容来自FunctionAST::codegen()
	//{{{
		//FunctionAST 其实有两个member，一个proto一个body，body就是BinaryExprAST E

	Function *TheFunction = TheModule->getFunction(Proto_Name);
		//根据教程，要首先去查是不是在TheModule里有预先定义过的叫Proto_Name的函数头。
		//因为Proto是一个我刚定义的函数体，可能前面有预先定义的函数头，叫同一个名字。假如搜不到，就用新拿到的函数头的型参列表来定义这个函数

  	
	if (!TheFunction) {
		//来自 PrototypeAST::codegen():
		//{{{
  		std::vector<Type *> Doubles(Proto_Args.size(), Type::getDoubleTy(TheContext));
		//FunctionType 应该是来自Function.h
 	 	FunctionType *FT = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

       	TheFunction = Function::Create(FT, Function::ExternalLinkage, Proto_Name, TheModule.get());

		unsigned Idx = 0;
		for (auto &Arg : TheFunction->args())
			Arg.setName(Proto_Args[Idx++]);
		//}}}
	}

	BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
	Builder.SetInsertPoint(BB);
		//教程说，BasicBlock用来定义IR的Control Flow Graph

  	// Record the function arguments in the NamedValues map.
  	NamedValues.clear();
  	for (auto &Arg : TheFunction->args())
  	  	NamedValues[std::string(Arg.getName())] = &Arg;

	//来自BinaryExprAST E的codegen (Body->codegen())
	//{{{
	Value *L =	ConstantFP::get(TheContext, APFloat(LHS));
	Value *R =	ConstantFP::get(TheContext, APFloat(RHS));
	//}}}
	
   	Value *RetVal = Builder.CreateFAdd(L, R, "addtmp");

    Builder.CreateRet(RetVal);
    
	verifyFunction(*TheFunction);


    TheFunction->print(errs());
	//}}}

	//以下内容来自HandleTopLevelExpression
	//{{{
    auto H = TheJIT->addModule(std::move(TheModule));

    auto ExprSymbol = TheJIT->findSymbol("__anon_expr");
    assert(ExprSymbol && "Function not found");

    double (*FP)() = (double (*)())(intptr_t)cantFail(ExprSymbol.getAddress());
    fprintf(stderr, "Evaluated to %f\n", FP());

    TheJIT->removeModule(H);
	//}}}


	return 0;
}
