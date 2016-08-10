/*
 * util-ir.cpp
 *
 *  Created on: Jul 14, 2016
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
#include <stdlib.h>
#include <memory>
#include <iostream>

#include "../include/config.h"
#include "../include/Reader.h"
#include "../include/RLE2.h"
#include "../include/util.h"
#include "llvm-codegen.h"
#include "functions-ir.h"
#include "ColumnInfo.h"

using namespace llvm;

namespace orc{

Function *genfunc_unZigZag(LlvmCodeGen *gen,EncodingInfo info){

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"unZigZag",gen->int_type(64),true);
	proto.AddArgument("value",gen->int_type(64));
	Value *params[1];
	Function *fn = proto.GeneratePrototype(&builder,&params[0]);
	AllocaInst *value_addr = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(params[0],value_addr,8);
	Value *valuevalue = builder.CreateAlignedLoad(value_addr,8);
	Value *shiftvalue = builder.CreateLShr(valuevalue,gen->getConstant(64,1));
	Value *andvalue = builder.CreateAnd(valuevalue,gen->getConstant(64,1));
	Value *subvalue = builder.CreateSub(gen->getConstant(64,0),andvalue);
	Value *xorvalue = builder.CreateXor(shiftvalue,subvalue);
	builder.CreateRet(xorvalue);

	return fn;
}

Function *genfunc_decodeBitWidth(LlvmCodeGen *gen, EncodingInfo info){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"decodeBitWidth",gen->int_type(32),true);
	proto.AddArgument("n", gen->int_type(32));
	llvm::Value *params[1];
	Function *fn = proto.GeneratePrototype(&builder,&params[0]);
	uint32_t n_original[] = {23,24,25,26,27,28,29,30};
	uint32_t n_return[] = {24,26,28,30,32,40,48,56};

	BasicBlock *label_if_cond[8];
	BasicBlock *label_if_body[8];

	//Value *n_ptr = builder.CreateAlignedLoad(params[0],4);
	Value *n_value = params[0];
	for(int i=0;i<8;i++){
		char tmp[20];
		sprintf(tmp,"if.cond.%d",i);
		label_if_cond[i] = BasicBlock::Create(gen->context(), tmp,fn,0);
		sprintf(tmp,"if.body.%d",i);
		label_if_body[i] = BasicBlock::Create(gen->context(), tmp,fn,0);
	}
	BasicBlock *label_if_end = BasicBlock::Create(gen->context(), "if.end",fn,0);

	builder.CreateBr(label_if_cond[0]);
	builder.SetInsertPoint(label_if_cond[0]);
	{
		Value *cmp_value = builder.CreateICmpULE(n_value,gen->getConstant(32,23));
		builder.CreateCondBr(cmp_value,label_if_body[0],label_if_cond[1]);
	}
	builder.SetInsertPoint(label_if_body[0]);
	{
		builder.CreateRet(builder.CreateAdd(n_value,gen->getConstant(32,1)));
	}

	for(int i=1;i<8;i++){
		builder.SetInsertPoint(label_if_cond[i]);
		{
			Value *cmp_value = builder.CreateICmpEQ(n_value,gen->getConstant(32,n_original[i]));
			if(i!=7){
				builder.CreateCondBr(cmp_value,label_if_body[i],label_if_cond[i+1]);
			}else{
				builder.CreateCondBr(cmp_value,label_if_body[i],label_if_end);
			}
		}
		builder.SetInsertPoint(label_if_body[i]);
		{
			builder.CreateRet(gen->getConstant(32,n_return[i]));
		}
	}

	builder.SetInsertPoint(label_if_end);
	{
		builder.CreateRet(gen->getConstant(32,64));
	}
	//gen->AddFunctionToJit(fn,(void**)&fn);
	return fn;

}

Function *genfunc_readLongBE(LlvmCodeGen *gen, EncodingInfo info){

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"readLongBE",gen->int_type(64),true);
	proto.AddArgument("data",gen->int_ptr_type(8));
	proto.AddArgument("index",gen->int_ptr_type(64));
	proto.AddArgument("bytesize",gen->int_type(64));

	Value *params[3];
	Function *fn = proto.GeneratePrototype(&builder,&params[0]);


	BasicBlock* label_end = BasicBlock::Create(gen->context(), "end",fn,0);

	Value *data_ptr = params[0];
	Value *index_ptr = params[1];
	Value *bytesize_value = params[2];

	Value *ret_ptr = builder.CreateAlloca(gen->int_type(64));
	builder.CreateStore(gen->getConstant(64,0),ret_ptr);

	if(info.isbitwidthfixed){

		int bytewidth = info.bitwidth;
		Value *ret_value = builder.CreateAlignedLoad(ret_ptr,8);
		while (bytewidth > 0) {
		    bytewidth--;
			Value *index_value = builder.CreateAlignedLoad(index_ptr,8);
			Value *incvalue = builder.CreateAdd(index_value,gen->getConstant(64,1));
			builder.CreateAlignedStore(incvalue,index_ptr,8);
			Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_ptr,index_value);
			Value *datavalue = builder.CreateAlignedLoad(dataGEP,1);
			Value *datavalue_64 = builder.CreateSExt(datavalue,gen->int_type(64));
			//ret |= data[index]<<(n*8)
			Value *shiftvalue = builder.CreateShl(datavalue_64,gen->getConstant(64,bytewidth*8));
			ret_value = builder.CreateOr(ret_value,shiftvalue);
		}
		builder.CreateAlignedStore(ret_value,ret_ptr,8);
		builder.CreateBr(label_end);

	}else{
		//n = bytesize;
		Value *n_ptr = builder.CreateAlloca(gen->int_type(64));
		builder.CreateAlignedStore(bytesize_value,n_ptr,8);

		BasicBlock* label_while_cond = BasicBlock::Create(gen->context(), "while.cond",fn,0);
		BasicBlock* label_while_body = BasicBlock::Create(gen->context(), "while.body",fn,0);
		builder.CreateBr(label_while_cond);
		//while(n > 0)
		builder.SetInsertPoint(label_while_cond);
		{
			Value *n_value = builder.CreateAlignedLoad(n_ptr,8);
			Value *cmp = builder.CreateICmpUGT(n_value,gen->getConstant(64,0));
			builder.CreateCondBr(cmp,label_while_body,label_end);
		}
		builder.SetInsertPoint(label_while_body);
		{
			//n--;
			Value *n_value = builder.CreateAlignedLoad(n_ptr,8);
			n_value = builder.CreateSub(n_value,gen->getConstant(64,1));
			builder.CreateAlignedStore(n_value,n_ptr,8);

			//data[index++]
			Value *index_value = builder.CreateAlignedLoad(index_ptr,8);
			Value *incvalue = builder.CreateAdd(index_value,gen->getConstant(64,1));
			builder.CreateAlignedStore(incvalue,index_ptr,8);
			Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_ptr,index_value);
			Value *datavalue = builder.CreateAlignedLoad(dataGEP,1);
			Value *datavalue_64 = builder.CreateSExt(datavalue,gen->int_type(64));
			//ret |= data[index]<<(n*8)
			Value *ntimes8 = builder.CreateMul(n_value,gen->getConstant(64,8));
			Value *shiftvalue = builder.CreateShl(datavalue_64,ntimes8);
			Value *ret_value = builder.CreateAlignedLoad(ret_ptr,8);
			ret_value = builder.CreateOr(ret_value,shiftvalue);
			builder.CreateAlignedStore(ret_value,ret_ptr,8);
			builder.CreateBr(label_while_cond);
		}
	}


	builder.SetInsertPoint(label_end);
	//return ret;
	Value *retvalue = builder.CreateAlignedLoad(ret_ptr,8);
	builder.CreateRet(retvalue);
	return fn;

}

Function *genfunc_readVuLong(LlvmCodeGen *gen, EncodingInfo info){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"readVuLong",gen->int_type(64),true);
	proto.AddArgument("data",gen->int_ptr_type(8));
	proto.AddArgument("index",gen->int_ptr_type(64));
	Value *params[2];
	Function *fn = proto.GeneratePrototype(&builder,&params[0]);

	BasicBlock* label_do_body = BasicBlock::Create(gen->context(), "do.body",fn,0);
	BasicBlock* label_do_cond = BasicBlock::Create(gen->context(), "do.cond",fn,0);
	BasicBlock* label_do_end = BasicBlock::Create(gen->context(), "do.end",fn,0);

	Value *data_ptr = params[0];
	Value *index_ptr = params[1];

	//ret = 0;
	//offset = 0;
	Value *ret = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(gen->getConstant(64,0),ret,8);
	Value *offset = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(gen->getConstant(64,0),offset,8);
	Value *b = builder.CreateAlloca(gen->int_type(8));
	builder.CreateBr(label_do_body);

	builder.SetInsertPoint(label_do_body);
	{
		//b = data[index++];
		Value *index_value = builder.CreateAlignedLoad(index_ptr,8);
		Value *indexadd = builder.CreateAdd(index_value,gen->getConstant(64,1));
		builder.CreateAlignedStore(indexadd,index_ptr,8);
		Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_ptr,index_value);
		Value *b_value = builder.CreateAlignedLoad(dataGEP,1);
		builder.CreateAlignedStore(b_value,b,1);
		//ret = ret | (0x7f & b) << offset
		Value *ret_value = builder.CreateAlignedLoad(ret,8);
		Value *andvalue = builder.CreateAnd(b_value,gen->getConstant(8,0x7f));
		Value *andvalue_64 = builder.CreateSExt(andvalue,gen->int_type(64));
		Value *offset_value = builder.CreateAlignedLoad(offset,8);
		Value *shiftvalue = builder.CreateShl(andvalue_64,offset_value);
		Value *orvalue = builder.CreateOr(ret_value,shiftvalue);
		builder.CreateAlignedStore(orvalue,ret,8);
		//offset += 7;
		Value *incvalue = builder.CreateAdd(offset_value,gen->getConstant(64,7));
		builder.CreateAlignedStore(incvalue,offset,8);
		builder.CreateBr(label_do_cond);
	}
	//while(b >= 0x80)
	builder.SetInsertPoint(label_do_cond);
	{
		Value *b_value = builder.CreateAlignedLoad(b,1);
		Value *cmp_value = builder.CreateICmpUGE(b_value,gen->getConstant(8,0x80));
		builder.CreateCondBr(cmp_value,label_do_body,label_do_end);
	}

	//return ret;
	builder.SetInsertPoint(label_do_end);
	Value *retvalue = builder.CreateAlignedLoad(ret,8);
	builder.CreateRet(retvalue);

	return fn;

}

Function *genfunc_readVsLong(LlvmCodeGen *gen, EncodingInfo info){

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"readVsLong",IntegerType::getInt64Ty(gen->context()),true);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("index", gen->int_ptr_type(64));
	llvm::Value *params[2];
	Function *fn = proto.GeneratePrototype(&builder,&params[0]);

	Value *uvalue = builder.CreateCall(genfunc_readVuLong(gen, info),params);
	Value *svalue = builder.CreateCall(genfunc_unZigZag(gen,info),uvalue);
	builder.CreateRet(svalue);
	//gen->AddFunctionToJit(fn,(void **)&fn);

	return fn;
}

Function *genfunc_readLongs_test(LlvmCodeGen *gen, EncodingInfo info){

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"readLongs",IntegerType::getInt64Ty(gen->context()),true);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("result", gen->int_ptr_type(64));
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));
	proto.AddArgument("len",gen->int_type(64));
	proto.AddArgument("fb",gen->int_type(64));

	llvm::Value *params[5];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *data_ptr = params[0];
	Value *result_ptr = params[1];
	Value *index_ptr = params[2];
	Value *offset_value = params[3];
	Value *len_value = params[4];
	Value *fb_value = params[5];

	BasicBlock* label_for_cond = BasicBlock::Create(gen->context(), "for.cond",fn,0);
	BasicBlock* label_for_body = BasicBlock::Create(gen->context(), "for.body",fn,0);

	BasicBlock* label_for_inc = BasicBlock::Create(gen->context(), "for.inc",fn,0);
	BasicBlock* label_for_end = BasicBlock::Create(gen->context(), "for.end",fn,0);


	Value *i_ptr = builder.CreateAlloca(builder.getInt64Ty());
	Value *bitsLeft_value;
	Value *curByte_value;
	Value *i_value;
	Value *rawresult_value;
	Value *bitsLeftToRead_value;
	Value *index_value;// = builder.CreateAlignedLoad(index_ptr,8);

	//bitsLeft = 0;
	bitsLeft_value = gen->getConstant(64,0);
	Value *end_value;
	{
		//i = offset
		//end = len+offset;
		builder.CreateAlignedStore(builder.getInt64(0),i_ptr,8);
		end_value = builder.CreateAdd(offset_value,len_value);
		builder.CreateBr(label_for_cond);
	}

	builder.SetInsertPoint(label_for_cond);
	{
		//i<end;
		i_value = builder.CreateAlignedLoad(i_ptr,8);
		Value *cmpvalue = builder.CreateICmpULT(i_value,end_value);
		builder.CreateCondBr(cmpvalue,label_for_body,label_for_end);
	}
	builder.SetInsertPoint(label_for_body);
	{
		//rawresult = 0;
		rawresult_value = gen->getConstant(64,0);
	}
	/*
	 *
	 * rawresult <<= 8;
		rawresult |= data[index++]&0xff;
	 * */
	if(info.isbitwidthfixed&&info.bitwidth>=8){
		if(true){

			index_value = builder.CreateAlignedLoad(index_ptr,8);
			for(int i=0;i<info.bitwidth/8;i++){
				Value *shiftvalue;
				if(i==0){
					shiftvalue = rawresult_value;
				}else{
					shiftvalue = builder.CreateShl(rawresult_value,gen->getConstant(64,8));
				}
				Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_ptr,index_value);
				Value *datavalue = builder.CreateAlignedLoad(dataGEP,1);
				index_value = builder.CreateAdd(index_value,gen->getConstant(64,1));

				Value *datavalue_64 = builder.CreateSExt(datavalue,gen->int_type(64));
				Value *andvalue = builder.CreateAnd(datavalue_64,gen->getConstant(64,0xff));
				rawresult_value = builder.CreateOr(shiftvalue,andvalue);
			}
			builder.CreateAlignedStore(index_value,index_ptr,8);
		}else{

		}
	}else{
		BasicBlock* label_if_then = BasicBlock::Create(gen->context(), "if.then",fn,0);
		BasicBlock* label_if_end = BasicBlock::Create(gen->context(), "if.end",fn,0);
		BasicBlock* label_while_cond = BasicBlock::Create(gen->context(), "while.cond",fn,0);
		BasicBlock* label_while_body = BasicBlock::Create(gen->context(), "while.body",fn,0);
		BasicBlock* label_while_end = BasicBlock::Create(gen->context(), "while.end",fn,0);
		BasicBlock* label_if_then_1 = BasicBlock::Create(gen->context(), "if.then.1",fn,0);
		BasicBlock* label_if_end_1 = BasicBlock::Create(gen->context(), "if.end.1",fn,0);

		{
			//bitsLeftToRead = fb;
			bitsLeftToRead_value = fb_value;
			//if(bitsLeft==0)
			Value *cmpvalue = builder.CreateICmpEQ(bitsLeft_value,gen->getConstant(64,0));
			builder.CreateCondBr(cmpvalue,label_if_then,label_if_end);
		}
		{
			//curByte = data[index++];
			//bitsLeft = 8;
			builder.SetInsertPoint(label_if_then);
			Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_ptr,index_value);
			Value *curByte_value = builder.CreateAlignedLoad(dataGEP,1);
			index_value = builder.CreateAdd(index_value,gen->getConstant(64,1));
			bitsLeft_value = gen->getConstant(64,8);
			builder.CreateBr(label_if_end);
		}
		builder.SetInsertPoint(label_if_end);
		builder.CreateBr(label_while_cond);
		{
			//while(bitsLeftToRead>bitsLeft)
			builder.SetInsertPoint(label_while_cond);
			Value *cmpvalue = builder.CreateICmpUGT(bitsLeftToRead_value,bitsLeft_value);
			builder.CreateCondBr(cmpvalue,label_while_body,label_while_end);
		}

		builder.SetInsertPoint(label_while_body);
		{
			//rawresult<<bitsLeft;

			Value *shiftvalue = builder.CreateShl(rawresult_value,bitsLeft_value);
			//rawresult = (rawresult<<bitsLeft) | (curByte & (1<<bitsLeft-1))
			Value *curBytevalue_64 = builder.CreateSExt(curByte_value,gen->int_type(64));
			Value *shiftvalue1 = builder.CreateShl(gen->getConstant(64,1),bitsLeft_value);
			Value *subvalue = builder.CreateSub(shiftvalue1,gen->getConstant(64,1));
			Value *andvalue = builder.CreateAnd(curBytevalue_64,subvalue);
			rawresult_value = builder.CreateOr(shiftvalue,andvalue);
			//bitsLeftToRead -= bitsLeft;
			bitsLeftToRead_value = builder.CreateSub(bitsLeftToRead_value,bitsLeft_value);
			//curByte = data[index++];
			//bitsLeft = 8;
			Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_ptr,index_value);
			curByte_value = builder.CreateAlignedLoad(dataGEP,1);
			index_value = builder.CreateAdd(index_value,gen->getConstant(64,1));
			bitsLeft_value = gen->getConstant(64,8);
			builder.CreateBr(label_while_cond);
		}

		builder.SetInsertPoint(label_while_end);
		{
			//if(bitsLeftToRead > 0)
			Value *cmpvalue = builder.CreateICmpUGT(bitsLeftToRead_value,gen->getConstant(64,0));
			builder.CreateCondBr(cmpvalue,label_if_then_1,label_if_end_1);
		}
		{
			//rawresult <<= bitsLeftToRead;
			builder.SetInsertPoint(label_if_then_1);
			rawresult_value = builder.CreateShl(rawresult_value,bitsLeftToRead_value);
			//bitsLeft -= bitsLeftToRead;
			bitsLeft_value = builder.CreateSub(bitsLeft_value,bitsLeftToRead_value);
			//rawresult |= (curByte>>bitsLeft)&(1<<bitsLeftToRead - 1)
			Value *curBytevalue_64 = builder.CreateSExt(curByte_value,gen->int_type(64));
			Value *shiftvalue1 = builder.CreateAShr(curBytevalue_64,bitsLeft_value);
			Value *shiftvalue2 = builder.CreateShl(gen->getConstant(64,1),bitsLeftToRead_value);
			Value *subvalue1 = builder.CreateSub(shiftvalue2,gen->getConstant(64,1));
			Value *andvalue = builder.CreateAnd(shiftvalue1,subvalue1);
			rawresult_value = builder.CreateOr(rawresult_value,andvalue);
			builder.CreateBr(label_if_end_1);
		}
		builder.SetInsertPoint(label_if_end_1);
	}
	{
		Value *unzigzagedvalue;
		if(info.issigned){
			unzigzagedvalue = builder.CreateCall(genfunc_unZigZag(gen,info),rawresult_value);
		}else{
			unzigzagedvalue = rawresult_value;
		}
		Value *resultGEP = builder.CreateGEP(gen->int_type(64),result_ptr,i_value);
		builder.CreateAlignedStore(unzigzagedvalue,resultGEP,8);
		builder.CreateBr(label_for_inc);
	}

	builder.SetInsertPoint(label_for_inc);
	{
		i_value = builder.CreateAlignedLoad(i_ptr,8);
		i_value = builder.CreateAdd(i_value,gen->getConstant(64,1));
		builder.CreateAlignedStore(i_value,i_ptr,8);
		builder.CreateBr(label_for_cond);
	}

	builder.SetInsertPoint(label_for_end);
	//builder.CreateAlignedStore(index_value,index_ptr,8);
	builder.CreateRet(len_value);
	fn->dump();

	//gen->AddFunctionToJit(fn,(void **)&fn);
	return fn;

}

Function *genfunc_readLongs(LlvmCodeGen *gen, EncodingInfo info){

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"readLongs",IntegerType::getInt64Ty(gen->context()),true);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));
	proto.AddArgument("len",gen->int_type(64));
	proto.AddArgument("bitwidth",gen->int_type(64));
	proto.AddArgument("result",gen->int_ptr_type(64));

	llvm::Value *params[6];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *data_ptr = params[0];
	Value *index_ptr = params[1];
	Value *offset_value = params[2];
	Value *len_value = params[3];
	Value *bitwidth_value = params[4];
	Value *result_ptr = params[5];


	BasicBlock* label_for_cond = BasicBlock::Create(gen->context(), "for.cond",fn,0);
	BasicBlock* label_for_body = BasicBlock::Create(gen->context(), "for.body",fn,0);

	BasicBlock* label_for_inc = BasicBlock::Create(gen->context(), "for.inc",fn,0);
	BasicBlock* label_for_end = BasicBlock::Create(gen->context(), "for.end",fn,0);


	Value *bitsLeft_ptr = builder.CreateAlloca(gen->int_type(64));
	Value *curByte_ptr = builder.CreateAlloca(gen->int_type(8));
	Value *i_ptr = builder.CreateAlloca(gen->int_type(64));
	Value *rawresult_ptr = builder.CreateAlloca(gen->int_type(64));
	Value *bitsLeftToRead_ptr = builder.CreateAlloca(gen->int_type(64));

	builder.CreateAlignedStore(gen->getConstant(64,0),bitsLeft_ptr,8);
	Value *end_value;
	{
		//i = offset
		//end = len+offset;
		builder.CreateAlignedStore(offset_value,i_ptr,8);
		end_value = builder.CreateAdd(offset_value,len_value);
		builder.CreateBr(label_for_cond);
	}

	builder.SetInsertPoint(label_for_cond);
	{
		//i<end;
		Value *i_value = builder.CreateAlignedLoad(i_ptr,8);
		Value *cmpvalue = builder.CreateICmpULT(i_value,end_value);
		builder.CreateCondBr(cmpvalue,label_for_body,label_for_end);
	}
	{
	//rawresult = 0;
	//bitsLeftToRead = fb;
	builder.SetInsertPoint(label_for_body);
	builder.CreateAlignedStore(gen->getConstant(64,0),rawresult_ptr,8);
	}
	/*
	 *
	 * rawresult <<= 8;
		rawresult |= data[index++]&0xff;
	 * */
	if(info.isbitwidthfixed&&info.bitwidth>=8){
		if(true){
			Value *index_value = builder.CreateAlignedLoad(index_ptr,8);
			Value *rawresult_value = builder.getInt64(0);
			for(int i=0;i<info.bitwidth/8;i++){
				Value *shiftvalue;
				if(i==0){
					shiftvalue = rawresult_value;
				}else{
					shiftvalue = builder.CreateShl(rawresult_value,gen->getConstant(64,8));
				}
				Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_ptr,index_value);
				Value *datavalue = builder.CreateAlignedLoad(dataGEP,1);
				index_value = builder.CreateAdd(index_value,gen->getConstant(64,1));

				Value *datavalue_64 = builder.CreateSExt(datavalue,gen->int_type(64));
				Value *andvalue = builder.CreateAnd(datavalue_64,gen->getConstant(64,0xff));
				rawresult_value = builder.CreateOr(shiftvalue,andvalue);
			}
			builder.CreateAlignedStore(index_value,index_ptr,8);
			builder.CreateAlignedStore(rawresult_value,rawresult_ptr,8);
		}else{

		}
	}else{
		BasicBlock* label_if_then = BasicBlock::Create(gen->context(), "if.then",fn,0);
		BasicBlock* label_if_end = BasicBlock::Create(gen->context(), "if.end",fn,0);
		BasicBlock* label_while_cond = BasicBlock::Create(gen->context(), "while.cond",fn,0);
		BasicBlock* label_while_body = BasicBlock::Create(gen->context(), "while.body",fn,0);
		BasicBlock* label_while_end = BasicBlock::Create(gen->context(), "while.end",fn,0);
		BasicBlock* label_if_then_1 = BasicBlock::Create(gen->context(), "if.then.1",fn,0);
		BasicBlock* label_if_end_1 = BasicBlock::Create(gen->context(), "if.end.1",fn,0);

		{
			builder.CreateAlignedStore(bitwidth_value,bitsLeftToRead_ptr,8);
			//if(bitsLeft==0)
			Value *bitsLeft_value = builder.CreateAlignedLoad(bitsLeft_ptr,8);
			Value *cmpvalue = builder.CreateICmpEQ(bitsLeft_value,gen->getConstant(64,0));
			builder.CreateCondBr(cmpvalue,label_if_then,label_if_end);
		}
		{
			//curByte = data[index++];
			//bitsLeft = 8;
			builder.SetInsertPoint(label_if_then);
			Value *index_value = builder.CreateAlignedLoad(index_ptr,8);
			Value *incvalue = builder.CreateAdd(index_value,gen->getConstant(64,1));
			builder.CreateAlignedStore(incvalue,index_ptr,8);
			Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_ptr,index_value);
			Value *datavalue = builder.CreateAlignedLoad(dataGEP,1);
			builder.CreateAlignedStore(datavalue,curByte_ptr,1);
			builder.CreateAlignedStore(gen->getConstant(64,8), bitsLeft_ptr,8);
			builder.CreateBr(label_if_end);
		}
		builder.SetInsertPoint(label_if_end);
		builder.CreateBr(label_while_cond);
		{
			//while(bitsLeftToRead>bitsLeft)
			builder.SetInsertPoint(label_while_cond);
			Value *bitsLeftToRead_value = builder.CreateAlignedLoad(bitsLeftToRead_ptr,8);
			Value *bitsLeft_value = builder.CreateAlignedLoad(bitsLeft_ptr,8);
			Value *cmpvalue = builder.CreateICmpUGT(bitsLeftToRead_value,bitsLeft_value);
			builder.CreateCondBr(cmpvalue,label_while_body,label_while_end);
		}

		builder.SetInsertPoint(label_while_body);
		{
			//rawresult<<bitsLeft;
			Value *bitsLeft_value = builder.CreateAlignedLoad(bitsLeft_ptr,8);
			Value *rawresult_value = builder.CreateAlignedLoad(rawresult_ptr,8);
			Value *shiftvalue = builder.CreateShl(rawresult_value,bitsLeft_value);
			//rawresult = (rawresult<<bitsLeft) | (curByte & (1<<bitsLeft-1))
			Value *curByte_value = builder.CreateAlignedLoad(curByte_ptr,1);
			Value *curBytevalue_64 = builder.CreateSExt(curByte_value,gen->int_type(64));
			Value *shiftvalue1 = builder.CreateShl(gen->getConstant(64,1),bitsLeft_value);
			Value *subvalue = builder.CreateSub(shiftvalue1,gen->getConstant(64,1));
			Value *andvalue = builder.CreateAnd(curBytevalue_64,subvalue);
			Value *orvalue = builder.CreateOr(shiftvalue,andvalue);
			builder.CreateAlignedStore(orvalue,rawresult_ptr,8);
			//bitsLeftToRead -= bitsLeft;
			Value *bitsLeftToRead_value = builder.CreateAlignedLoad(bitsLeftToRead_ptr,8);
			Value *subvalue1 = builder.CreateSub(bitsLeftToRead_value,bitsLeft_value);
			builder.CreateAlignedStore(subvalue1,bitsLeftToRead_ptr,8);
			//curByte = data[index++];
			//bitsLeft = 8;
			Value *index_value = builder.CreateAlignedLoad(index_ptr,8);
			Value *incvalue = builder.CreateAdd(index_value,gen->getConstant(64,1));
			builder.CreateAlignedStore(incvalue,index_ptr,8);
			Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_ptr,index_value);
			Value *datavalue = builder.CreateAlignedLoad(dataGEP,1);
			builder.CreateAlignedStore(datavalue,curByte_ptr,8);
			builder.CreateAlignedStore(gen->getConstant(64,8),bitsLeft_ptr,8);
			builder.CreateBr(label_while_cond);
		}

		builder.SetInsertPoint(label_while_end);
		{
			//if(bitsLeftToRead > 0)
			Value *bitsLeftToRead_value = builder.CreateAlignedLoad(bitsLeftToRead_ptr,8);
			Value *cmpvalue = builder.CreateICmpUGT(bitsLeftToRead_value,gen->getConstant(64,0));
			builder.CreateCondBr(cmpvalue,label_if_then_1,label_if_end_1);
		}

		builder.SetInsertPoint(label_if_then_1);
		{
			//rawresult <<= bitsLeftToRead;
			Value *bitsLeftToRead_value = builder.CreateAlignedLoad(bitsLeftToRead_ptr,8);
			Value *rawresult_value = builder.CreateAlignedLoad(rawresult_ptr,8);
			rawresult_value = builder.CreateShl(rawresult_value,bitsLeftToRead_value);
			//bitsLeft -= bitsLeftToRead;
			Value *bitsLeft_value = builder.CreateAlignedLoad(bitsLeft_ptr,8);
			Value *subvalue = builder.CreateSub(bitsLeft_value,bitsLeftToRead_value);
			builder.CreateAlignedStore(subvalue,bitsLeft_ptr,8);
			//rawresult |= (curByte>>bitsLeft)&(1<<bitsLeftToRead - 1)
			Value *curByte_value = builder.CreateAlignedLoad(curByte_ptr,1);
			Value *curBytevalue_64 = builder.CreateSExt(curByte_value,gen->int_type(64));
			Value *shiftvalue1 = builder.CreateAShr(curBytevalue_64,subvalue);
			Value *shiftvalue2 = builder.CreateShl(gen->getConstant(64,1),bitsLeftToRead_value);
			Value *subvalue1 = builder.CreateSub(shiftvalue2,gen->getConstant(64,1));
			Value *andvalue = builder.CreateAnd(shiftvalue1,subvalue1);
			rawresult_value = builder.CreateOr(rawresult_value,andvalue);
			builder.CreateAlignedStore(rawresult_value,rawresult_ptr,8);
			builder.CreateBr(label_if_end_1);
		}
		builder.SetInsertPoint(label_if_end_1);
	}
	{
		Value *pos_value = builder.CreateAlignedLoad(i_ptr,8);
		Value *rawresult_value = builder.CreateAlignedLoad(rawresult_ptr,8);
		if(info.issigned){
			rawresult_value = builder.CreateCall(genfunc_unZigZag(gen,info),rawresult_value);
		}
		Value *resultGEP = builder.CreateGEP(gen->int_type(64),result_ptr,pos_value);
		builder.CreateAlignedStore(rawresult_value,resultGEP,8);

		builder.CreateBr(label_for_inc);
	}

	builder.SetInsertPoint(label_for_inc);
	{
		Value *i_value = builder.CreateAlignedLoad(i_ptr,8);
		i_value = builder.CreateAdd(i_value,gen->getConstant(64,1));
		builder.CreateAlignedStore(i_value,i_ptr,8);
		builder.CreateBr(label_for_cond);
	}

	builder.SetInsertPoint(label_for_end);
	builder.CreateRet(len_value);

	//gen->AddFunctionToJit(fn,(void **)&fn);
	return fn;

}
}
