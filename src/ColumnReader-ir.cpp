/*
 * ColumnReader-ir.cpp
 *
 *  Created on: Jul 24, 2016
 *      Author: teng
 */


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
#include "ColumnInfo.h"
#include "functions-ir.h"
#include "MemSpace.h"

using namespace llvm;

namespace orc{


Function *genfunc_readDouble(LlvmCodeGen *gen){

	PointerType *space_ptr_type = orc::MemSpace::getMemSpacePtr(gen);

	Type *int_ptr = PointerType::get(PointerType::getInt64PtrTy(gen->context(),0),0);
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"readDouble",IntegerType::getVoidTy(gen->context()),false);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));
	proto.AddArgument("length",gen->int_type(64));
	proto.AddArgument("space",space_ptr_type);

	llvm::Value *params[5];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *data_addr = params[0];
	Value *index_addr = params[1];
	Value *offset_value = params[2];
	Value *length_value = params[3];
	Value *space_ptr = params[4];

	Value *end_value = builder.CreateAdd(offset_value,length_value);
	Value *t_ptr = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(offset_value,t_ptr,8);

	BasicBlock *label_for_cond = BasicBlock::Create(gen->context(),"for.cond",fn);
	BasicBlock *label_for_body = BasicBlock::Create(gen->context(),"for.body",fn);
	BasicBlock *label_for_inc = BasicBlock::Create(gen->context(),"for.inc",fn);
	BasicBlock *label_for_end = BasicBlock::Create(gen->context(),"for.end",fn);

	builder.CreateBr(label_for_cond);
	builder.SetInsertPoint(label_for_cond);
	{
		Value *t_value = builder.CreateAlignedLoad(t_ptr,8);
		Value *cmp_value = builder.CreateICmpULT(t_value,end_value);
		builder.CreateCondBr(cmp_value,label_for_body,label_for_end);
	}

	builder.SetInsertPoint(label_for_body);
	{
		Value *double_value;
		{
			Value *indexvalue = builder.CreateAlignedLoad(index_addr,8);
			Value *bits_value = builder.getInt64(0);
			for(int i=0;i<8;i++){
				Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_addr,indexvalue);
				indexvalue = builder.CreateAdd(indexvalue,gen->getConstant(64,1));
				Value *data_value = builder.CreateAlignedLoad(dataGEP,1);
				Value *data_value_64 = builder.CreateZExt(data_value,gen->int_type(64));
				Value *shiftvalue = builder.CreateShl(data_value_64,gen->getConstant(64,i*8));
				bits_value = builder.CreateOr(bits_value,shiftvalue);
			}
			builder.CreateAlignedStore(indexvalue,index_addr,8);
			double_value = builder.CreateBitCast(bits_value,gen->double_type());
		}

		//process(double rowvalue, double offset, TempSpace &space)
		{
			Value *offset_value = builder.CreateAlignedLoad(t_ptr,8);
			orc::processValue(gen, builder,double_value,offset_value,space_ptr);
		}
		builder.CreateBr(label_for_inc);
	}

	builder.SetInsertPoint(label_for_inc);
	{
		Value *t_value = builder.CreateAlignedLoad(t_ptr,8);
		Value *incvalue = builder.CreateAdd(t_value,gen->getConstant(64,1));
		builder.CreateAlignedStore(incvalue,t_ptr,8);
		builder.CreateBr(label_for_cond);
	}

	builder.SetInsertPoint(label_for_end);
	builder.CreateRetVoid();

	gen->AddFunctionToJit(fn,(void **)&fn);
	//fn->dump();
	return fn;


}



}
