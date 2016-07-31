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

using namespace llvm;

namespace orc{


Function *genfunc_readDouble(LlvmCodeGen *gen, ColumnInfo info){
	Type *double_ptr = PointerType::get(PointerType::getDoublePtrTy(gen->context(),0),0);
	Type *int_ptr = PointerType::get(PointerType::getInt64PtrTy(gen->context(),0),0);
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"nextFunc_"+info.colname,IntegerType::getVoidTy(gen->context()),false);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("result", gen->double_ptr_type());
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));
	proto.AddArgument("length",gen->int_type(64));
	proto.AddArgument("bitmap",builder.getInt8PtrTy());
	proto.AddArgument("IRresult",double_ptr);
	proto.AddArgument("GPresult",int_ptr);
	proto.AddArgument("aggresult",double_ptr);

	llvm::Value *params[9];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *data_addr = params[0];
	Value *result_addr = params[1];
	Value *index_addr = params[2];
	Value *offset_value = params[3];
	Value *length_value = params[4];
	Value *bitmap_addr = params[5];
	Value *IRresult_addr = params[6];
	Value *GPresult_addr = params[7];
	Value *aggresult_addr = params[8];

	Value *bits_ptr = builder.CreateAlloca(gen->int_type(64));
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
		builder.CreateAlignedStore(gen->getConstant(64,0),bits_ptr,8);
		for(int i=0;i<8;i++){
			Value *indexvalue = builder.CreateAlignedLoad(index_addr,8);
			Value *incvalue = builder.CreateAdd(indexvalue,gen->getConstant(64,1));
			builder.CreateAlignedStore(incvalue,index_addr,8);
			Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_addr,indexvalue);
			Value *data_value = builder.CreateAlignedLoad(dataGEP,1);
			Value *data_value_64 = builder.CreateZExt(data_value,gen->int_type(64));
			Value *shiftvalue = builder.CreateShl(data_value_64,gen->getConstant(64,i*8));
			Value *bits_value = builder.CreateAlignedLoad(bits_ptr,8);
			Value *orvalue = builder.CreateOr(bits_value,shiftvalue);
			builder.CreateAlignedStore(orvalue,bits_ptr,8);
		}

		Value *bits_value = builder.CreateAlignedLoad(bits_ptr,8);
		Value *double_value = builder.CreateBitCast(bits_value,gen->double_type());

		Value *t_value = builder.CreateAlignedLoad(t_ptr,8);

		if(info.updateAggregation){
			//aggpos = hash(uint64_t offset, uint64_t **GPresult);
			Function *hash_func;
			Value *aggpos;
			{
				std::vector<LlvmCodeGen::NamedVariable> vars;
				vars.push_back(LlvmCodeGen::NamedVariable("offset",gen->int_type(64)));
				vars.push_back(LlvmCodeGen::NamedVariable("GPresult",int_ptr));
				hash_func = genfunc_slot(gen,vars,builder.getInt64Ty(),"hash",true);
				std::vector<Value *> call_params;
				call_params.push_back(t_value);
				call_params.push_back(GPresult_addr);
				aggpos = builder.CreateCall(hash_func,call_params);
			}
			Function *process_func;
			//process(double rowvalue, double **IRresult, double **aggresult, uint64_t offset, uint64_t aggpos)
			{
				std::vector<LlvmCodeGen::NamedVariable> vars;
				vars.push_back(LlvmCodeGen::NamedVariable("rowvalue",gen->double_type()));
				vars.push_back(LlvmCodeGen::NamedVariable("IRresult",double_ptr));
				vars.push_back(LlvmCodeGen::NamedVariable("aggresult",double_ptr));
				vars.push_back(LlvmCodeGen::NamedVariable("offset",gen->int_type(64)));
				vars.push_back(LlvmCodeGen::NamedVariable("aggpos",gen->int_type(64)));

				process_func = genfunc_slot(gen,vars,builder.getVoidTy(),"process",true);
				std::vector<Value *> call_params;
				call_params.push_back(double_value);
				call_params.push_back(IRresult_addr);
				call_params.push_back(aggresult_addr);
				call_params.push_back(t_value);
				call_params.push_back(aggpos);
				builder.CreateCall(process_func,call_params);
			}
		}

		if(info.saveResult){
			Value *resultGEP = builder.CreateGEP(gen->double_type(),result_addr,t_value);
			builder.CreateStore(double_value,resultGEP);
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
