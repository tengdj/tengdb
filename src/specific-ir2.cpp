// Generated by llvm2cpp - DO NOT MODIFY!

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
#include "llvm-codegen.h"
#include "processColumn.hh"

using namespace llvm;

namespace orc{

Function* genfunc_specific(LlvmCodeGen *gen, std::vector<ColumnPattern> patterns) {
 
 // Type Definitions
 std::vector<Type*>FuncTy_0_args;
 PointerType* PointerTy_1 = PointerType::get(IntegerType::get(gen->context(), 8), 0);
 
 FuncTy_0_args.push_back(PointerTy_1);
 PointerType* PointerTy_2 = PointerType::get(IntegerType::get(gen->context(), 64), 0);
 
 FuncTy_0_args.push_back(PointerTy_2);
 FuncTy_0_args.push_back(PointerTy_2);
 FunctionType* FuncTy_0 = FunctionType::get(
  /*Result=*/IntegerType::get(gen->context(), 64),
  /*Params=*/FuncTy_0_args,
  /*isVarArg=*/false);

 // Function Declarations
 
 Function* func_readDirect_codegen = Function::Create(
  /*Type=*/FuncTy_0,
  /*Linkage=*/GlobalValue::ExternalLinkage,
  /*Name=*/"readDirect_codegen", gen->module());
 func_readDirect_codegen->setCallingConv(CallingConv::C);

 AttributeSet func_readDirect_codegen_PAL;
 {
  SmallVector<AttributeSet, 4> Attrs;
  AttributeSet PAS;
   {
    AttrBuilder B;
    PAS = AttributeSet::get(gen->context(), 3U, B);
   }
  
  Attrs.push_back(PAS);
  {
   AttrBuilder B;
   PAS = AttributeSet::get(gen->context(), 4U, B);
  }
 
 Attrs.push_back(PAS);
 {
  AttrBuilder B;
  B.addAttribute(Attribute::NoUnwind);
  B.addAttribute(Attribute::UWTable);
  PAS = AttributeSet::get(gen->context(), ~0U, B);
 }

Attrs.push_back(PAS);
func_readDirect_codegen_PAL = AttributeSet::get(gen->context(), Attrs);

}
func_readDirect_codegen->setAttributes(func_readDirect_codegen_PAL);

// Global Variable Declarations


// Constant Definitions
ConstantInt* const_int32_6 = ConstantInt::get(gen->context(), APInt(32, StringRef("1"), 10));
ConstantInt* const_int64_7 = ConstantInt::get(gen->context(), APInt(64, StringRef("1"), 10));
ConstantInt* const_int32_8 = ConstantInt::get(gen->context(), APInt(32, StringRef("8"), 10));
ConstantInt* const_int32_9 = ConstantInt::get(gen->context(), APInt(32, StringRef("0"), 10));
ConstantInt* const_int64_0 = ConstantInt::get(gen->context(), APInt(64, StringRef("0"), 10));

ConstantInt* const_int32_10 = ConstantInt::get(gen->context(), APInt(32, StringRef("255"), 10));

// Global Variable Definitions

// Function Definitions

// Function: readDirect_codegen (func_readDirect_codegen)
{
 Function::arg_iterator args = func_readDirect_codegen->arg_begin();
 Value* ptr_data = args++;
 ptr_data->setName("data");
 Value* ptr_result = args++;
 ptr_result->setName("result");
 Value* ptr_index = args++;
 ptr_index->setName("index");
 
 BasicBlock* label_entry = BasicBlock::Create(gen->context(), "entry",func_readDirect_codegen,0);
 BasicBlock* label_for_cond = BasicBlock::Create(gen->context(), "for.cond",func_readDirect_codegen,0);
 BasicBlock* label_for_body = BasicBlock::Create(gen->context(), "for.body",func_readDirect_codegen,0);
 BasicBlock* label_for_inc = BasicBlock::Create(gen->context(), "for.inc",func_readDirect_codegen,0);
 BasicBlock* label_for_end = BasicBlock::Create(gen->context(), "for.end",func_readDirect_codegen,0);
 
 // Block entry (label_entry)
 AllocaInst* ptr_data_addr = new AllocaInst(PointerTy_1, "data.addr", label_entry);
 ptr_data_addr->setAlignment(8);
 AllocaInst* ptr_result_addr = new AllocaInst(PointerTy_2, "result.addr", label_entry);
 ptr_result_addr->setAlignment(8);
 AllocaInst* ptr_index_addr = new AllocaInst(PointerTy_2, "index.addr", label_entry);
 ptr_index_addr->setAlignment(8);

 AllocaInst* ptr_resultindex = new AllocaInst(IntegerType::get(gen->context(), 64), "resultindex", label_entry);
 ptr_resultindex->setAlignment(8);

 AllocaInst* ptr_firstbyte = new AllocaInst(IntegerType::get(gen->context(), 8), "firstbyte", label_entry);
 ptr_firstbyte->setAlignment(1);
 AllocaInst* ptr_secondbyte = new AllocaInst(IntegerType::get(gen->context(), 8), "secondbyte", label_entry);
 ptr_secondbyte->setAlignment(1);
 AllocaInst* ptr_runLength = new AllocaInst(IntegerType::get(gen->context(), 32), "runLength", label_entry);
 ptr_runLength->setAlignment(4);

 AllocaInst* ptr_rawresult = new AllocaInst(IntegerType::get(gen->context(), 32), "rawresult", label_entry);
 ptr_rawresult->setAlignment(4);

 StoreInst* void_11 = new StoreInst(ptr_data, ptr_data_addr, false, label_entry);
 void_11->setAlignment(8);
 StoreInst* void_12 = new StoreInst(ptr_result, ptr_result_addr, false, label_entry);
 void_12->setAlignment(8);
 StoreInst* void_13 = new StoreInst(ptr_index, ptr_index_addr, false, label_entry);
 void_13->setAlignment(8);

 StoreInst* void_14 = new StoreInst(const_int64_0, ptr_resultindex, false, label_entry);
 void_14->setAlignment(8);

 LoadInst* ptr_15 = new LoadInst(ptr_index_addr, "", false, label_entry);
 ptr_15->setAlignment(8);
 LoadInst* int64_16 = new LoadInst(ptr_15, "", false, label_entry);
 int64_16->setAlignment(8);
 BinaryOperator* int64_inc = BinaryOperator::Create(Instruction::Add, int64_16, const_int64_7, "inc", label_entry);
 StoreInst* void_17 = new StoreInst(int64_inc, ptr_15, false, label_entry);
 void_17->setAlignment(8);
 LoadInst* ptr_18 = new LoadInst(ptr_data_addr, "", false, label_entry);
 ptr_18->setAlignment(8);
 GetElementPtrInst* ptr_arrayidx = GetElementPtrInst::Create(IntegerType::getInt8Ty(gen->context()),ptr_18, int64_16, "arrayidx", label_entry);
 LoadInst* int8_19 = new LoadInst(ptr_arrayidx, "", false, label_entry);
 int8_19->setAlignment(1);
 StoreInst* void_20 = new StoreInst(int8_19, ptr_firstbyte, false, label_entry);
 void_20->setAlignment(1);
 LoadInst* ptr_21 = new LoadInst(ptr_index_addr, "", false, label_entry);
 ptr_21->setAlignment(8);
 LoadInst* int64_22 = new LoadInst(ptr_21, "", false, label_entry);
 int64_22->setAlignment(8);
 BinaryOperator* int64_inc1 = BinaryOperator::Create(Instruction::Add, int64_22, const_int64_7, "inc1", label_entry);
 StoreInst* void_23 = new StoreInst(int64_inc1, ptr_21, false, label_entry);
 void_23->setAlignment(8);
 LoadInst* ptr_24 = new LoadInst(ptr_data_addr, "", false, label_entry);
 ptr_24->setAlignment(8);
 GetElementPtrInst* ptr_arrayidx2 = GetElementPtrInst::Create(IntegerType::getInt8Ty(gen->context()),ptr_24, int64_22, "arrayidx2", label_entry);
 LoadInst* int8_25 = new LoadInst(ptr_arrayidx2, "", false, label_entry);
 int8_25->setAlignment(1);
 StoreInst* void_26 = new StoreInst(int8_25, ptr_secondbyte, false, label_entry);
 void_26->setAlignment(1);
 LoadInst* int8_27 = new LoadInst(ptr_firstbyte, "", false, label_entry);
 int8_27->setAlignment(1);
 CastInst* int32_conv = new SExtInst(int8_27, IntegerType::get(gen->context(), 32), "conv", label_entry);
 BinaryOperator* int32_and = BinaryOperator::Create(Instruction::And, int32_conv, const_int32_6, "and", label_entry);
 BinaryOperator* int32_shl = BinaryOperator::Create(Instruction::Shl, int32_and, const_int32_8, "shl", label_entry);
 StoreInst* void_28 = new StoreInst(int32_shl, ptr_runLength, false, label_entry);
 void_28->setAlignment(4);
 LoadInst* int8_29 = new LoadInst(ptr_secondbyte, "", false, label_entry);
 int8_29->setAlignment(1);
 CastInst* int32_conv3 = new ZExtInst(int8_29, IntegerType::get(gen->context(), 32), "conv3", label_entry);
 LoadInst* int32_30 = new LoadInst(ptr_runLength, "", false, label_entry);
 int32_30->setAlignment(4);
 BinaryOperator* int32_or = BinaryOperator::Create(Instruction::Or, int32_30, int32_conv3, "or", label_entry);
 StoreInst* void_31 = new StoreInst(int32_or, ptr_runLength, false, label_entry);
 void_31->setAlignment(4);
 LoadInst* int32_32 = new LoadInst(ptr_runLength, "", false, label_entry);
 int32_32->setAlignment(4);
 BinaryOperator* int32_add = BinaryOperator::Create(Instruction::Add, int32_32, const_int32_6, "add", label_entry);
 StoreInst* void_33 = new StoreInst(int32_add, ptr_runLength, false, label_entry);
 void_33->setAlignment(4);

 BranchInst::Create(label_for_cond, label_entry);
 
 AllocaInst* ptr_tt = new AllocaInst(IntegerType::get(gen->context(), 32), "tt", label_entry);
 ptr_tt->setAlignment(4);

 for(ColumnPattern pattern:patterns){
 

	StoreInst* void_34 = new StoreInst(const_int32_9, ptr_tt, false, label_entry);
	void_34->setAlignment(4);

	// Block for.cond (label_for_cond)
	LoadInst* int32_36 = new LoadInst(ptr_tt, "", false, label_for_cond);
	int32_36->setAlignment(4);
	LoadInst* int32_37 = new LoadInst(ptr_runLength, "", false, label_for_cond);
	int32_37->setAlignment(4);
	ICmpInst* int1_cmp = new ICmpInst(*label_for_cond, ICmpInst::ICMP_ULT, int32_36, int32_37, "cmp");
	BranchInst::Create(label_for_body, label_for_end, int1_cmp, label_for_cond);

	// Block for.body (label_for_body)
	//rawresult = 0;
	StoreInst* void_39 = new StoreInst(const_int32_9, ptr_rawresult, false, label_for_body);
	void_39->setAlignment(4);

	for(int i=0;i<3;i++)
	{
	 //index++
	 LoadInst* ptr_40 = new LoadInst(ptr_index_addr, "", false, label_for_body);
	 ptr_40->setAlignment(8);
	 LoadInst* int64_41 = new LoadInst(ptr_40, "", false, label_for_body);
	 int64_41->setAlignment(8);
	 BinaryOperator* int64_inc4 = BinaryOperator::Create(Instruction::Add, int64_41, const_int64_7, "inc4", label_for_body);
	 StoreInst* void_42 = new StoreInst(int64_inc4, ptr_40, false, label_for_body);
	 void_42->setAlignment(8);
	 //data[index++]
	 LoadInst* ptr_43 = new LoadInst(ptr_data_addr, "", false, label_for_body);
	 ptr_43->setAlignment(8);
	 GetElementPtrInst* ptr_arrayidx5 = GetElementPtrInst::Create(IntegerType::getInt8Ty(gen->context()),ptr_43, int64_41, "arrayidx5", label_for_body);
	 LoadInst* int8_44 = new LoadInst(ptr_arrayidx5, "", false, label_for_body);
	 int8_44->setAlignment(1);
	 CastInst* int32_conv6 = new SExtInst(int8_44, IntegerType::get(gen->context(), 32), "conv6", label_for_body);

	 BinaryOperator* int32_and7 = BinaryOperator::Create(Instruction::And, int32_conv6, const_int32_10, "and7", label_for_body);
	 LoadInst* int32_45 = new LoadInst(ptr_rawresult, "", false, label_for_body);
	 int32_45->setAlignment(4);
	 BinaryOperator* int32_or8 = BinaryOperator::Create(Instruction::Or, int32_45, int32_and7, "or8", label_for_body);
	 if(i<2){
		 //rawresult <<= 8;
		 BinaryOperator* int32_shl9 = BinaryOperator::Create(Instruction::Shl, int32_or8, const_int32_8, "shl9", label_for_body);
		 StoreInst* void_46 = new StoreInst(int32_shl9, ptr_rawresult, false, label_for_body);
		 void_46->setAlignment(4);
	 }else{
		 StoreInst* void_46 = new StoreInst(int32_or8, ptr_rawresult, false, label_for_body);
		 void_46->setAlignment(4);
	 }
	}

	//rawresult >> 1 ^ -(rawresult & 1)
	LoadInst* int32_65 = new LoadInst(ptr_rawresult, "", false, label_for_body);
	int32_65->setAlignment(4);
	BinaryOperator* int32_shr = BinaryOperator::Create(Instruction::LShr, int32_65, const_int32_6, "shr", label_for_body);

	BinaryOperator* int32_and21 = BinaryOperator::Create(Instruction::And, int32_65, const_int32_6, "and21", label_for_body);
	BinaryOperator* int32_sub = BinaryOperator::Create(Instruction::Sub, const_int32_9, int32_and21, "sub", label_for_body);

	BinaryOperator* int32_xor = BinaryOperator::Create(Instruction::Xor, int32_shr, int32_sub, "xor", label_for_body);
	CastInst* int64_conv22 = new ZExtInst(int32_xor, IntegerType::get(gen->context(), 64), "conv22", label_for_body);
	//result[resultindex++] =

	LoadInst* int64_68 = new LoadInst(ptr_resultindex, "", false, label_for_body);
	int64_68->setAlignment(8);
	BinaryOperator* int64_inc23 = BinaryOperator::Create(Instruction::Add, int64_68, const_int64_7, "inc23", label_for_body);
	StoreInst* void_69 = new StoreInst(int64_inc23, ptr_resultindex, false, label_for_body);
	void_69->setAlignment(8);

	LoadInst* ptr_70 = new LoadInst(ptr_result_addr, "", false, label_for_body);
	ptr_70->setAlignment(8);
	GetElementPtrInst* ptr_arrayidx24 = GetElementPtrInst::Create(IntegerType::getInt64Ty(gen->context()),ptr_70, int64_68, "arrayidx24", label_for_body);
	StoreInst* void_71 = new StoreInst(int64_conv22, ptr_arrayidx24, false, label_for_body);
	void_71->setAlignment(8);

	BranchInst::Create(label_for_inc, label_for_body);

	// Block for.inc (label_for_inc)
	LoadInst* int32_73 = new LoadInst(ptr_tt, "", false, label_for_inc);
	int32_73->setAlignment(4);
	BinaryOperator* int32_inc25 = BinaryOperator::Create(Instruction::Add, int32_73, const_int32_6, "inc25", label_for_inc);
	StoreInst* void_74 = new StoreInst(int32_inc25, ptr_tt, false, label_for_inc);
	void_74->setAlignment(4);


 }
 
 BranchInst::Create(label_for_cond, label_for_inc);
 
 // Block for.end (label_for_end)
 LoadInst* int32_76 = new LoadInst(ptr_runLength, "", false, label_for_end);
 int32_76->setAlignment(4);
 CastInst* int64_conv26 = new ZExtInst(int32_76, IntegerType::get(gen->context(), 64), "conv26", label_for_end);
 ReturnInst::Create(gen->context(), int64_conv26, label_for_end);
 
}

return func_readDirect_codegen;
}


}
