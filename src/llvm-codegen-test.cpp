/*
 * llvm-codegen-test.cc
 *
 *  Created on: Mar 17, 2016
 *      Author: teng
 */

#include "llvm-codegen.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

#include "object-pool.h"
#include <vector>
#include <stdint.h>
#include <iostream>

using namespace llvm;
using namespace std;
using namespace orc;


Function *genFunc(LlvmCodeGen *gen,Function *callee,const string funcname){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,funcname,IntegerType::getInt32Ty(gen->context()),false);
	proto.AddArgument("left",IntegerType::getInt32Ty(gen->context()));
	Value *args[1];
	Function *func = proto.GeneratePrototype(&builder,&args[0]);
	CallInst *call = builder.CreateCall(callee,args[0]);
	builder.CreateRet(call);
	return func;
}

int another(int t){
	return t+315;
}

int main(int argc, char **argv){

	ObjectPool pool;
	LlvmCodeGen* gen;
	LlvmCodeGen::InitializeLlvm();
	LlvmCodeGen::LoadFromFile(&pool,"/home/teng/orc/src/llvmir/test.bc","test",&gen);
	//unique_ptr<Module> secondmodule = LlvmCodeGen::LoadModuleFromFile(gen,"/home/teng/orc/src/llvmir/forlink.bc");
	gen->LinkModule("/home/teng/orc/src/llvmir/forlink.bc");
	if(!gen)return 0;

	Function *AddOneF =
		        cast<Function>(gen->module()->getOrInsertFunction("addOne", IntegerType::getInt32Ty(gen->context()), IntegerType::getInt32Ty(gen->context()),
		                                              (llvm::Type *)0));
	AddOneF->setCallingConv(CallingConv::C);
	gen->execution_engine()->addGlobalMapping(AddOneF, (void*)&another);

	Function *genfunc = genFunc(gen,AddOneF,"genfunc");
	gen->FinalizeFunction(genfunc);

	Function *genfuncwrap = genFunc(gen,genfunc,"genfuncwrap");
	gen->AddFunctionToJit(genfunc,(void**)&genfunc);
	gen->AddFunctionToJit(genfuncwrap,(void **)&genfuncwrap);


	vector<Function*> functions;
	gen->GetFunctions(&functions);
	Function *fn = functions[5];

	int nr = 0;
	Function *newfunc = gen->ReplaceCallSites(fn,false,genfuncwrap,"forreplace",&nr);
	gen->InlineCallSites(genfuncwrap,true);
	gen->FinalizeModule();


	uint64_t fnaddr = gen->GetFunction(newfunc->getName().str());
	int (*func)(int) = (int (*)(int))(fnaddr);
	cout<<func(10)<<endl;

	//gen->module()->dump();
	return 1;
}


