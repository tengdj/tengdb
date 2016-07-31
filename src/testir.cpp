// Generated by llvm2cpp - DO NOT MODIFY!

#include <llvm/Pass.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/Verifier.h>
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
using namespace llvm;

Module* makeLLVMModule();

int main(int argc, char**argv) {
  Module* Mod = makeLLVMModule();
  verifyModule(*Mod, PrintMessageAction);
  PassManager PM;
  PM.add(createPrintModulePass(&outs()));
  PM.run(*Mod);
  return 0;
}


Module* makeLLVMModule() {
 // Module Construction
 Module* mod = new Module("testir.ll", getGlobalContext());
 mod->setDataLayout("");
 mod->setTargetTriple("x86_64-unknown-linux-gnu");
 
 // Type Definitions
 std::vector<Type*>FuncTy_0_args;
 PointerType* PointerTy_2 = PointerType::get(Type::getDoubleTy(mod->getContext()), 0);
 
 PointerType* PointerTy_1 = PointerType::get(PointerTy_2, 0);
 
 FuncTy_0_args.push_back(PointerTy_1);
 FuncTy_0_args.push_back(PointerTy_1);
 FunctionType* FuncTy_0 = FunctionType::get(
  /*Result=*/Type::getDoubleTy(mod->getContext()),
  /*Params=*/FuncTy_0_args,
  /*isVarArg=*/false);
 
 PointerType* PointerTy_3 = PointerType::get(PointerTy_1, 0);
 
 
 // Function Declarations
 
 Function* func__Z4testPPdS0_ = mod->getFunction("_Z4testPPdS0_");
 if (!func__Z4testPPdS0_) {
 func__Z4testPPdS0_ = Function::Create(
  /*Type=*/FuncTy_0,
  /*Linkage=*/GlobalValue::ExternalLinkage,
  /*Name=*/"_Z4testPPdS0_", mod); 
 func__Z4testPPdS0_->setCallingConv(CallingConv::C);
 }
 AttributeSet func__Z4testPPdS0__PAL;
 {
  SmallVector<AttributeSet, 4> Attrs;
  AttributeSet PAS;
   {
    AttrBuilder B;
    B.addAttribute(Attribute::NoUnwind);
    B.addAttribute(Attribute::UWTable);
    PAS = AttributeSet::get(mod->getContext(), ~0U, B);
   }
  
  Attrs.push_back(PAS);
  func__Z4testPPdS0__PAL = AttributeSet::get(mod->getContext(), Attrs);
  
 }
 func__Z4testPPdS0_->setAttributes(func__Z4testPPdS0__PAL);
 
 // Global Variable Declarations

 
 // Constant Definitions
 ConstantInt* const_int32_4 = ConstantInt::get(mod->getContext(), APInt(32, StringRef("1"), 10));
 ConstantFP* const_double_5 = ConstantFP::get(mod->getContext(), APFloat(0.000000e+00));
 ConstantInt* const_int64_6 = ConstantInt::get(mod->getContext(), APInt(64, StringRef("10"), 10));
 
 // Global Variable Definitions
 
 // Function Definitions
 
 // Function: _Z4testPPdS0_ (func__Z4testPPdS0_)
 {
  Function::arg_iterator args = func__Z4testPPdS0_->arg_begin();
  Value* ptr_aggresult = args++;
  ptr_aggresult->setName("aggresult");
  Value* ptr_result = args++;
  ptr_result->setName("result");
  
  BasicBlock* label_entry = BasicBlock::Create(mod->getContext(), "entry",func__Z4testPPdS0_,0);
  
  // Block entry (label_entry)
  AllocaInst* ptr_aggresult_addr = new AllocaInst(PointerTy_1, "aggresult.addr", label_entry);
  ptr_aggresult_addr->setAlignment(8);
  AllocaInst* ptr_result_addr = new AllocaInst(PointerTy_1, "result.addr", label_entry);
  ptr_result_addr->setAlignment(8);
  AllocaInst* ptr_i = new AllocaInst(Type::getDoubleTy(mod->getContext()), "i", label_entry);
  ptr_i->setAlignment(8);
  StoreInst* void_7 = new StoreInst(ptr_aggresult, ptr_aggresult_addr, false, label_entry);
  void_7->setAlignment(8);
  StoreInst* void_8 = new StoreInst(ptr_result, ptr_result_addr, false, label_entry);
  void_8->setAlignment(8);
  StoreInst* void_9 = new StoreInst(const_double_5, ptr_i, false, label_entry);
  void_9->setAlignment(8);
  LoadInst* ptr_10 = new LoadInst(ptr_result_addr, "", false, label_entry);
  ptr_10->setAlignment(8);
  GetElementPtrInst* ptr_arrayidx = GetElementPtrInst::Create(ptr_10, const_int64_6, "arrayidx", label_entry);
  LoadInst* ptr_11 = new LoadInst(ptr_arrayidx, "", false, label_entry);
  ptr_11->setAlignment(8);
  GetElementPtrInst* ptr_arrayidx1 = GetElementPtrInst::Create(ptr_11, const_int64_6, "arrayidx1", label_entry);
  LoadInst* double_12 = new LoadInst(ptr_arrayidx1, "", false, label_entry);
  double_12->setAlignment(8);
  LoadInst* double_13 = new LoadInst(ptr_i, "", false, label_entry);
  double_13->setAlignment(8);
  BinaryOperator* double_add = BinaryOperator::Create(Instruction::FAdd, double_12, double_13, "add", label_entry);
  LoadInst* ptr_14 = new LoadInst(ptr_aggresult_addr, "", false, label_entry);
  ptr_14->setAlignment(8);
  GetElementPtrInst* ptr_arrayidx2 = GetElementPtrInst::Create(ptr_14, const_int64_6, "arrayidx2", label_entry);
  LoadInst* ptr_15 = new LoadInst(ptr_arrayidx2, "", false, label_entry);
  ptr_15->setAlignment(8);
  GetElementPtrInst* ptr_arrayidx3 = GetElementPtrInst::Create(ptr_15, const_int64_6, "arrayidx3", label_entry);
  LoadInst* double_16 = new LoadInst(ptr_arrayidx3, "", false, label_entry);
  double_16->setAlignment(8);
  BinaryOperator* double_add4 = BinaryOperator::Create(Instruction::FAdd, double_add, double_16, "add4", label_entry);
  ReturnInst::Create(mod->getContext(), double_add4, label_entry);
  
 }
 
 return mod;
}