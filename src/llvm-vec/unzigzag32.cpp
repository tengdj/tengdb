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
 Module* mod = new Module("unzigzag32.ll", getGlobalContext());
 mod->setDataLayout("");
 mod->setTargetTriple("x86_64-unknown-linux-gnu");
 
 // Type Definitions
 VectorType* VectorTy_0 = VectorType::get(IntegerType::get(mod->getContext(), 64), 4);
 
 std::vector<Type*>FuncTy_1_args;
 FuncTy_1_args.push_back(VectorTy_0);
 FuncTy_1_args.push_back(VectorTy_0);
 FuncTy_1_args.push_back(VectorTy_0);
 FunctionType* FuncTy_1 = FunctionType::get(
  /*Result=*/VectorTy_0,
  /*Params=*/FuncTy_1_args,
  /*isVarArg=*/false);
 
 PointerType* PointerTy_2 = PointerType::get(VectorTy_0, 0);
 
 PointerType* PointerTy_3 = PointerType::get(IntegerType::get(mod->getContext(), 32), 0);
 

 
 PointerType* PointerTy_5 = PointerType::get(FuncTy_6, 0);
 
 
 // Function Declarations
 
 Function* func__Z10unZigZag32Dv4_xS_S_ = mod->getFunction("_Z10unZigZag32Dv4_xS_S_");
 if (!func__Z10unZigZag32Dv4_xS_S_) {
 func__Z10unZigZag32Dv4_xS_S_ = Function::Create(
  /*Type=*/FuncTy_1,
  /*Linkage=*/GlobalValue::ExternalLinkage,
  /*Name=*/"_Z10unZigZag32Dv4_xS_S_", mod); 
 func__Z10unZigZag32Dv4_xS_S_->setCallingConv(CallingConv::C);
 }
 AttributeSet func__Z10unZigZag32Dv4_xS_S__PAL;
 {
  SmallVector<AttributeSet, 4> Attrs;
  AttributeSet PAS;
   {
    AttrBuilder B;
    B.addAttribute(Attribute::UWTable);
    PAS = AttributeSet::get(mod->getContext(), ~0U, B);
   }
  
  Attrs.push_back(PAS);
  func__Z10unZigZag32Dv4_xS_S__PAL = AttributeSet::get(mod->getContext(), Attrs);
  
 }
 func__Z10unZigZag32Dv4_xS_S_->setAttributes(func__Z10unZigZag32Dv4_xS_S__PAL);
 
 VectorType* VectorTy_4 = VectorType::get(IntegerType::get(mod->getContext(), 32), 8);

 std::vector<Type*>FuncTy_6_args;
 FuncTy_6_args.push_back(VectorTy_4);
 FuncTy_6_args.push_back(IntegerType::get(mod->getContext(), 32));
 FunctionType* FuncTy_6 = FunctionType::get(
  /*Result=*/VectorTy_4,
  /*Params=*/FuncTy_6_args,
  /*isVarArg=*/false);
 Function* func_llvm_x86_avx2_psrai_d = mod->getFunction("llvm.x86.avx2.psrai.d");
 if (!func_llvm_x86_avx2_psrai_d) {
 func_llvm_x86_avx2_psrai_d = Function::Create(
  /*Type=*/FuncTy_6,
  /*Linkage=*/GlobalValue::ExternalLinkage,
  /*Name=*/"llvm.x86.avx2.psrai.d", mod); // (external, no body)
 func_llvm_x86_avx2_psrai_d->setCallingConv(CallingConv::C);
 }
 AttributeSet func_llvm_x86_avx2_psrai_d_PAL;
 {
  SmallVector<AttributeSet, 4> Attrs;
  AttributeSet PAS;
   {
    AttrBuilder B;
    B.addAttribute(Attribute::NoUnwind);
    B.addAttribute(Attribute::ReadNone);
    PAS = AttributeSet::get(mod->getContext(), ~0U, B);
   }
  
  Attrs.push_back(PAS);
  func_llvm_x86_avx2_psrai_d_PAL = AttributeSet::get(mod->getContext(), Attrs);
  
 }
 func_llvm_x86_avx2_psrai_d->setAttributes(func_llvm_x86_avx2_psrai_d_PAL);
 
 // Global Variable Declarations

 
 // Constant Definitions
 ConstantInt* const_int32_7 = ConstantInt::get(mod->getContext(), APInt(32, StringRef("1"), 10));
 
 // Global Variable Definitions
 
 // Function Definitions
 
 // Function: _Z10unZigZag32Dv4_xS_S_ (func__Z10unZigZag32Dv4_xS_S_)
 {
  Function::arg_iterator args = func__Z10unZigZag32Dv4_xS_S_->arg_begin();
  Value* packed_result = args++;
  packed_result->setName("result");
  Value* packed_one_32 = args++;
  packed_one_32->setName("one_32");
  Value* packed_zero_32 = args++;
  packed_zero_32->setName("zero_32");
  
  BasicBlock* label_8 = BasicBlock::Create(mod->getContext(), "",func__Z10unZigZag32Dv4_xS_S_,0);
  
  // Block  (label_8)
  AllocaInst* ptr_9 = new AllocaInst(VectorTy_0, "", label_8);
  ptr_9->setAlignment(32);
  AllocaInst* ptr_10 = new AllocaInst(VectorTy_0, "", label_8);
  ptr_10->setAlignment(32);
  AllocaInst* ptr_11 = new AllocaInst(VectorTy_0, "", label_8);
  ptr_11->setAlignment(32);
  AllocaInst* ptr_12 = new AllocaInst(IntegerType::get(mod->getContext(), 32), "", label_8);
  ptr_12->setAlignment(4);
  AllocaInst* ptr_13 = new AllocaInst(VectorTy_0, "", label_8);
  ptr_13->setAlignment(32);
  AllocaInst* ptr_14 = new AllocaInst(VectorTy_0, "", label_8);
  ptr_14->setAlignment(32);
  AllocaInst* ptr_15 = new AllocaInst(VectorTy_0, "", label_8);
  ptr_15->setAlignment(32);
  AllocaInst* ptr_16 = new AllocaInst(VectorTy_0, "", label_8);
  ptr_16->setAlignment(32);
  AllocaInst* ptr_17 = new AllocaInst(VectorTy_0, "", label_8);
  ptr_17->setAlignment(32);
  AllocaInst* ptr_18 = new AllocaInst(VectorTy_0, "", label_8);
  ptr_18->setAlignment(32);
  AllocaInst* ptr_19 = new AllocaInst(VectorTy_0, "", label_8);
  ptr_19->setAlignment(32);
  AllocaInst* ptr_and_value = new AllocaInst(VectorTy_0, "and_value", label_8);
  ptr_and_value->setAlignment(32);
  AllocaInst* ptr_sub_value = new AllocaInst(VectorTy_0, "sub_value", label_8);
  ptr_sub_value->setAlignment(32);
  AllocaInst* ptr_shift_value = new AllocaInst(VectorTy_0, "shift_value", label_8);
  ptr_shift_value->setAlignment(32);
  StoreInst* void_20 = new StoreInst(packed_result, ptr_17, false, label_8);
  void_20->setAlignment(32);
  StoreInst* void_21 = new StoreInst(packed_one_32, ptr_18, false, label_8);
  void_21->setAlignment(32);
  StoreInst* void_22 = new StoreInst(packed_zero_32, ptr_19, false, label_8);
  void_22->setAlignment(32);
  LoadInst* packed_23 = new LoadInst(ptr_17, "", false, label_8);
  packed_23->setAlignment(32);
  LoadInst* packed_24 = new LoadInst(ptr_18, "", false, label_8);
  packed_24->setAlignment(32);
  StoreInst* void_25 = new StoreInst(packed_23, ptr_15, false, label_8);
  void_25->setAlignment(32);
  StoreInst* void_26 = new StoreInst(packed_24, ptr_16, false, label_8);
  void_26->setAlignment(32);
  LoadInst* packed_27 = new LoadInst(ptr_15, "", false, label_8);
  packed_27->setAlignment(32);
  LoadInst* packed_28 = new LoadInst(ptr_16, "", false, label_8);
  packed_28->setAlignment(32);
  BinaryOperator* packed_29 = BinaryOperator::Create(Instruction::And, packed_27, packed_28, "", label_8);
  StoreInst* void_30 = new StoreInst(packed_29, ptr_and_value, false, label_8);
  void_30->setAlignment(32);
  LoadInst* packed_31 = new LoadInst(ptr_19, "", false, label_8);
  packed_31->setAlignment(32);
  LoadInst* packed_32 = new LoadInst(ptr_and_value, "", false, label_8);
  packed_32->setAlignment(32);
  StoreInst* void_33 = new StoreInst(packed_31, ptr_9, false, label_8);
  void_33->setAlignment(32);
  StoreInst* void_34 = new StoreInst(packed_32, ptr_10, false, label_8);
  void_34->setAlignment(32);
  LoadInst* packed_35 = new LoadInst(ptr_9, "", false, label_8);
  packed_35->setAlignment(32);
  CastInst* packed_36 = new BitCastInst(packed_35, VectorTy_4, "", label_8);
  LoadInst* packed_37 = new LoadInst(ptr_10, "", false, label_8);
  packed_37->setAlignment(32);
  CastInst* packed_38 = new BitCastInst(packed_37, VectorTy_4, "", label_8);
  BinaryOperator* packed_39 = BinaryOperator::Create(Instruction::Sub, packed_36, packed_38, "", label_8);
  CastInst* packed_40 = new BitCastInst(packed_39, VectorTy_0, "", label_8);
  StoreInst* void_41 = new StoreInst(packed_40, ptr_sub_value, false, label_8);
  void_41->setAlignment(32);
  LoadInst* packed_42 = new LoadInst(ptr_17, "", false, label_8);
  packed_42->setAlignment(32);
  StoreInst* void_43 = new StoreInst(packed_42, ptr_11, false, label_8);
  void_43->setAlignment(32);
  StoreInst* void_44 = new StoreInst(const_int32_7, ptr_12, false, label_8);
  void_44->setAlignment(4);
  LoadInst* packed_45 = new LoadInst(ptr_11, "", false, label_8);
  packed_45->setAlignment(32);
  CastInst* packed_46 = new BitCastInst(packed_45, VectorTy_4, "", label_8);
  LoadInst* int32_47 = new LoadInst(ptr_12, "", false, label_8);
  int32_47->setAlignment(4);
  std::vector<Value*> packed_48_params;
  packed_48_params.push_back(packed_46);
  packed_48_params.push_back(int32_47);
  CallInst* packed_48 = CallInst::Create(func_llvm_x86_avx2_psrai_d, packed_48_params, "", label_8);
  packed_48->setCallingConv(CallingConv::C);
  packed_48->setTailCall(false);
  AttributeSet packed_48_PAL;
  {
   SmallVector<AttributeSet, 4> Attrs;
   AttributeSet PAS;
    {
     AttrBuilder B;
     B.addAttribute(Attribute::NoUnwind);
     PAS = AttributeSet::get(mod->getContext(), ~0U, B);
    }
   
   Attrs.push_back(PAS);
   packed_48_PAL = AttributeSet::get(mod->getContext(), Attrs);
   
  }
  packed_48->setAttributes(packed_48_PAL);
  
  CastInst* packed_49 = new BitCastInst(packed_48, VectorTy_0, "", label_8);
  StoreInst* void_50 = new StoreInst(packed_49, ptr_shift_value, false, label_8);
  void_50->setAlignment(32);
  LoadInst* packed_51 = new LoadInst(ptr_shift_value, "", false, label_8);
  packed_51->setAlignment(32);
  LoadInst* packed_52 = new LoadInst(ptr_sub_value, "", false, label_8);
  packed_52->setAlignment(32);
  StoreInst* void_53 = new StoreInst(packed_51, ptr_13, false, label_8);
  void_53->setAlignment(32);
  StoreInst* void_54 = new StoreInst(packed_52, ptr_14, false, label_8);
  void_54->setAlignment(32);
  LoadInst* packed_55 = new LoadInst(ptr_13, "", false, label_8);
  packed_55->setAlignment(32);
  LoadInst* packed_56 = new LoadInst(ptr_14, "", false, label_8);
  packed_56->setAlignment(32);
  BinaryOperator* packed_57 = BinaryOperator::Create(Instruction::Xor, packed_55, packed_56, "", label_8);
  ReturnInst::Create(mod->getContext(), packed_57, label_8);
  
 }
 
 return mod;
}