/*
 * Reader-IR.cpp
 *
 *  Created on: Jun 25, 2016
 *      Author: teng
 */

#include <stdint.h>

#include <llvm/Pass.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MathExtras.h>
#include <algorithm>

#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/ManagedStatic.h>
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/InstrTypes.h"

using namespace llvm;

namespace orc{

Module* makeLLVMModule_general();
//Module* makeLLVMModule_specific();

uint64_t getfunc(){
	//initialize LLVM utilization
		  InitializeNativeTarget();
		  InitializeNativeTargetAsmPrinter();
		  InitializeNativeTargetAsmParser();
		  std::unique_ptr<Module> module = std::unique_ptr<Module>(makeLLVMModule_general());
		  Module *m = module.get();
		  //module->dump();
		  std::unique_ptr<legacy::FunctionPassManager> TheFPM = llvm::make_unique<legacy::FunctionPassManager>(module.get());
		  // Do simple "peephole" optimizations and bit-twiddling optzns.
		  TheFPM->add(createInstructionCombiningPass());
		  // Reassociate expressions.
		  TheFPM->add(createReassociatePass());
		  // Eliminate Common SubExpressions.
		  TheFPM->add(createGVNPass());
		  // Simplify the control flow graph (deleting unreachable blocks, etc).
		  TheFPM->add(createCFGSimplificationPass());

		  TheFPM->doInitialization();
		  TheFPM->run(*module->getFunction("readDirect_codegen"));

		  //create a JIT engine to run time compile LLVM IR into executable instructions
		  std::string errmessage;
		  llvm::ExecutionEngine *ee =
					llvm::EngineBuilder(std::move(module)).setOptLevel(CodeGenOpt::Aggressive)
					.setErrorStr(&errmessage)
					.setEngineKind(EngineKind::JIT)
					.setMCJITMemoryManager(std::unique_ptr<RTDyldMemoryManager>(new SectionMemoryManager()))
					.create();
		  if(!ee){
			  printf("got an error while build engine: %s\n",errmessage.c_str());
			  exit(0);
		  }
		  uint64_t r = ee->getFunctionAddress("readDirect_codegen");
		  return r;
}
}

