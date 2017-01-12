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

#include "ORCColumnInfo.h"
#include "ORCReader.h"
#include "config.h"
#include "RLE2.h"
#include "util.h"
#include "llvm-codegen.h"
#include "functions-ir.h"
#include "vectorization-ir.h"

using namespace llvm;

namespace orc{

void advance(LlvmCodeGen *gen, IRBuilder<> &builder, Value *data_ptr, Value *index_ptr, Value *bytes[], int byte_num){

	Value *index_value = builder.CreateAlignedLoad(index_ptr,8);
	for(int i=0;i<byte_num;i++){
		Value *byte_GEP = builder.CreateGEP(gen->int_type(8),data_ptr,index_value);
		bytes[i] = builder.CreateAlignedLoad(byte_GEP,1);
		index_value = builder.CreateAdd(index_value,gen->getConstant(64,1));
		//print(builder,gen,"index: %d\n",index_value);

	}

	builder.CreateAlignedStore(index_value,index_ptr,8);
}

Function *genfunc_unZigZag(LlvmCodeGen *gen, int result_bitwidth){

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"unZigZag",gen->int_type(result_bitwidth),true);
	proto.AddArgument("value",gen->int_type(result_bitwidth));
	Value *params[1];
	Function *fn = proto.GeneratePrototype(&builder,&params[0]);
	AllocaInst *value_addr = builder.CreateAlloca(gen->int_type(result_bitwidth));
	builder.CreateAlignedStore(params[0],value_addr,8);
	Value *valuevalue = builder.CreateAlignedLoad(value_addr,8);
	Value *shiftvalue = builder.CreateLShr(valuevalue,gen->getConstant(result_bitwidth,1));
	Value *andvalue = builder.CreateAnd(valuevalue,gen->getConstant(result_bitwidth,1));
	Value *subvalue = builder.CreateSub(gen->getConstant(result_bitwidth,0),andvalue);
	Value *xorvalue = builder.CreateXor(shiftvalue,subvalue);
	builder.CreateRet(xorvalue);

	return fn;
}



Function *genfunc_readLongBE(LlvmCodeGen *gen, EncodingInfo *info, int result_bitwidth){

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"readLongBE",gen->int_type(result_bitwidth),true);
	proto.AddArgument("data",gen->int_ptr_type(8));
	proto.AddArgument("index",gen->int_ptr_type(64));
	proto.AddArgument("bytesize",gen->int_type(result_bitwidth));

	Value *params[3];
	Function *fn = proto.GeneratePrototype(&builder,&params[0]);
	BasicBlock* label_end = BasicBlock::Create(gen->context(), "end",fn,0);

	Value *data_ptr = params[0];
	Value *index_ptr = params[1];
	Value *bytesize_value = params[2];

	Value *ret_ptr = builder.CreateAlloca(gen->int_type(result_bitwidth));
	builder.CreateStore(gen->getConstant(result_bitwidth,0),ret_ptr);

//	if(info->isBitSizeFixed()){
//
//		int bytewidth = info->getFixedBitSize();
//		Value *ret_value = builder.CreateAlignedLoad(ret_ptr,8);
//		cout<<bytewidth<<endl;
//		while (bytewidth > 0) {
//		    bytewidth--;
//
//			Value *datavalue[1];
//			advance(gen,builder,data_ptr, index_ptr, datavalue,1);
//			Value *datavalue_ext = builder.CreateZExt(datavalue[0],gen->int_type(result_bitwidth));
//			//ret |= data[index]<<(n*8)
//			if(bytewidth!=0){
//				Value *shiftvalue = builder.CreateShl(datavalue_ext,gen->getConstant(result_bitwidth,bytewidth*8));
//				ret_value = builder.CreateOr(ret_value,shiftvalue);
//			}else{
//				ret_value = datavalue_ext;
//			}
//		}
//		builder.CreateAlignedStore(ret_value,ret_ptr,8);
//		builder.CreateBr(label_end);
//
//	}else
	{
		//n = bytesize;
		Value *n_ptr = builder.CreateAlloca(gen->int_type(result_bitwidth));

		builder.CreateAlignedStore(bytesize_value,n_ptr,8);

		BasicBlock* label_while_cond = BasicBlock::Create(gen->context(), "while.cond",fn,0);
		BasicBlock* label_while_body = BasicBlock::Create(gen->context(), "while.body",fn,0);
		builder.CreateBr(label_while_cond);
		//while(n > 0)
		builder.SetInsertPoint(label_while_cond);
		{
			Value *n_value = builder.CreateAlignedLoad(n_ptr,8);
			Value *cmp = builder.CreateICmpUGT(n_value,gen->getConstant(result_bitwidth,0));
			builder.CreateCondBr(cmp,label_while_body,label_end);
		}
		builder.SetInsertPoint(label_while_body);
		{
			//n--;
			Value *n_value = builder.CreateAlignedLoad(n_ptr,8);
			n_value = builder.CreateSub(n_value,gen->getConstant(result_bitwidth,1));
			builder.CreateAlignedStore(n_value,n_ptr,8);

			//data[index++]
			Value *datavalue[1];
			advance(gen,builder,data_ptr, index_ptr, datavalue,1);
			Value *datavalue_exit = builder.CreateZExt(datavalue[0],gen->int_type(result_bitwidth));
			//ret |= data[index]<<(n*8)
			Value *ntimes8 = builder.CreateMul(n_value,gen->getConstant(result_bitwidth,8));
			Value *shiftvalue = builder.CreateShl(datavalue_exit,ntimes8);
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

Function *genfunc_readVLong(LlvmCodeGen *gen, int result_bitwidth, bool issigned){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"readVuLong",gen->int_type(result_bitwidth),true);
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
	Value *ret = builder.CreateAlloca(gen->int_type(result_bitwidth));
	builder.CreateAlignedStore(gen->getConstant(result_bitwidth,0),ret,8);
	Value *offset_ptr = builder.CreateAlloca(gen->int_type(result_bitwidth));
	builder.CreateAlignedStore(gen->getConstant(result_bitwidth,0),offset_ptr,8);
	Value *b_ptr = builder.CreateAlloca(gen->int_type(8));
	builder.CreateBr(label_do_body);

	builder.SetInsertPoint(label_do_body);
	{
		//b = data[index++];
		Value *datavalue[1];
		advance(gen,builder,data_ptr, index_ptr, datavalue,1);
		Value *b_value = datavalue[0];
		builder.CreateAlignedStore(b_value,b_ptr,1);
		//ret = ret | (0x7f & b) << offset
		Value *ret_value = builder.CreateAlignedLoad(ret,8);
		Value *andvalue = builder.CreateAnd(b_value,gen->getConstant(8,0x7f));
		Value *andvalue_ext = builder.CreateSExt(andvalue,gen->int_type(result_bitwidth));
		Value *offset_value = builder.CreateAlignedLoad(offset_ptr,8);
		Value *shiftvalue = builder.CreateShl(andvalue_ext,offset_value);
		Value *orvalue = builder.CreateOr(ret_value,shiftvalue);
		builder.CreateAlignedStore(orvalue,ret,8);
		//offset += 7;
		Value *incvalue = builder.CreateAdd(offset_value,gen->getConstant(result_bitwidth,7));
		builder.CreateAlignedStore(incvalue,offset_ptr,8);
		builder.CreateBr(label_do_cond);
	}
	//while(b >= 0x80)
	builder.SetInsertPoint(label_do_cond);
	{
		Value *b_value = builder.CreateAlignedLoad(b_ptr,1);
		Value *cmp_value = builder.CreateICmpUGE(b_value,gen->getConstant(8,0x80));
		builder.CreateCondBr(cmp_value,label_do_body,label_do_end);
	}

	//return ret;
	builder.SetInsertPoint(label_do_end);
	Value *retvalue = builder.CreateAlignedLoad(ret,8);
	if(issigned){
		retvalue = builder.CreateCall(genfunc_unZigZag(gen,result_bitwidth),retvalue);
	}
	builder.CreateRet(retvalue);

	return fn;

}


//BasicBlock *readFoldedData(LlvmCodeGen *gen, IRBuilder<> &builder, Function *fn,int bitwidth, int runlength, bool issigned){
//
//	BasicBlock *label_folded_data = BasicBlock::Create(gen->context(),"",fn,0);
//	builder.SetInsertPoint(label_folded_data);
//
//	BasicBlock* label_for_cond = BasicBlock::Create(gen->context(), "for.cond",fn,0);
//	BasicBlock* label_for_body = BasicBlock::Create(gen->context(), "for.body",fn,0);
//	BasicBlock* label_for_inc = BasicBlock::Create(gen->context(), "for.inc",fn,0);
//
//	builder.CreateBr(label_for_cond);
//	builder.SetInsertPoint(label_for_cond);
//	{
//		//i<end;
//		Value *pos_value = builder.CreateAlignedLoad(offset_ptr,8);
//		Value *cmpvalue = builder.CreateICmpULT(pos_value,end_value);
//		builder.CreateCondBr(cmpvalue,label_for_body,label_end);
//	}
//	builder.SetInsertPoint(label_for_body);
//	{
//		Value *index_value = builder.CreateAlignedLoad(index_ptr,8);
//		Value *rawresult_value = builder.getInt64(0);
//		//read bytes
//		for(int i=0;i<info->getFixedBitSize()/8;i++){
//			Value *shiftvalue;
//			if(i==0){
//				shiftvalue = rawresult_value;
//			}else{
//				shiftvalue = builder.CreateShl(rawresult_value,gen->getConstant(64,8));
//			}
//			Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_ptr,index_value);
//			Value *datavalue = builder.CreateAlignedLoad(dataGEP,1);
//			index_value = builder.CreateAdd(index_value,gen->getConstant(64,1));
//
//			Value *datavalue_64 = builder.CreateSExt(datavalue,gen->int_type(64));
//			Value *andvalue = builder.CreateAnd(datavalue_64,gen->getConstant(64,0xff));
//			rawresult_value = builder.CreateOr(shiftvalue,andvalue);
//		}
//		//if signed unzigzag
//		if(info->issigned){
//			rawresult_value = builder.CreateCall(genfunc_unZigZag(gen),rawresult_value);
//		}
//		//store back index and result value
//		builder.CreateAlignedStore(index_value,index_ptr,8);
//
//		Value *pos_value = builder.CreateAlignedLoad(offset_ptr,8);
//		Value *resultGEP = builder.CreateGEP(gen->int_type(64),result_ptr,pos_value);
//		builder.CreateAlignedStore(rawresult_value,resultGEP,8);
//		builder.CreateBr(label_for_inc);
//	}
//	//increment result offset by 1
//	builder.SetInsertPoint(label_for_inc);
//	{
//		Value *pos_value = builder.CreateAlignedLoad(offset_ptr,8);
//		pos_value = builder.CreateAdd(pos_value,gen->getConstant(64,1));
//		builder.CreateAlignedStore(pos_value,offset_ptr,8);
//		builder.CreateBr(label_for_cond);
//	}
//
//
//
//	return label_folded_data;
//
//}

Function *genfunc_readLongs(LlvmCodeGen *gen, EncodingInfo *info, int result_bitwidth, bool issigned, bool optimize){

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"readLongs",IntegerType::getInt64Ty(gen->context()),true);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));
	proto.AddArgument("len",gen->int_type(64));
	proto.AddArgument("bitwidth",gen->int_type(result_bitwidth));
	proto.AddArgument("result",gen->int_ptr_type(result_bitwidth));

	llvm::Value *params[6];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *data_ptr = params[0];
	Value *index_ptr = params[1];
	Value *offset_value = params[2];
	Value *len_value = params[3];
	Value *bitwidth_value = params[4];
	Value *result_ptr = params[5];

	Value *pos_ptr = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(offset_value,pos_ptr,8);
	Value *end_value = builder.CreateAdd(offset_value,len_value);


	BasicBlock* label_end = BasicBlock::Create(gen->context(), "end",fn,0);


	if(optimize&&info->getFixedBitSize()>=8){

		if(info->getFixedBitSize()==24){
			VectorType* type_8_32 = VectorType::get(IntegerType::get(gen->context(), 8), 32);
			PointerType* type_8_32_pointer = PointerType::get(type_8_32, 0);
			VectorType* type_32_8 = VectorType::get(IntegerType::get(gen->context(), 32), 8);
			PointerType* type_32_8_pointer = PointerType::get(type_32_8, 0);
			int permute_mask[]{0,1,2,3,3,4,5,6};
			int shuffle_mask[]{2,1,0,-1,5,4,3,-1,8,7,6,-1,11,10,9,-1,2,1,0,-1,5,4,3,-1,8,7,6,-1,11,10,9,-1};
			Value *permute_mask_value = _mm256_set_epi32(gen,builder,permute_mask);
			Value *shuffle_mask_value = _mm256_set_epi8(gen,builder,shuffle_mask);
			int one_array[]{1,1,1,1,1,1,1,1};
			int zero_array[]{0,0,0,0,0,0,0,0};
			Value *one = _mm256_set_epi32(gen,builder,one_array);
			Value *zero = _mm256_set_epi32(gen,builder,zero_array);

			Value *runs = builder.CreateSDiv(len_value,builder.getInt64(8));
			Value *end_value = builder.CreateMul(runs,builder.getInt64(8));
			end_value = builder.CreateAdd(end_value,offset_value);
			BasicBlock *label_for_cond = BasicBlock::Create(gen->context(),"for.cond",fn);
			BasicBlock *label_for_body = BasicBlock::Create(gen->context(),"for.body",fn);
			BasicBlock *label_for_end = BasicBlock::Create(gen->context(),"for.end",fn);
			builder.CreateBr(label_for_cond);

			builder.SetInsertPoint(label_for_cond);
			{
				Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
				//print(gen,builder,"%d\n",pos_value);
				Value *cmp = builder.CreateICmpULT(pos_value,end_value);
				builder.CreateCondBr(cmp,label_for_body,label_for_end);
			}
			builder.SetInsertPoint(label_for_body);
			{
				//permute byte
				Value *index_value = builder.CreateAlignedLoad(index_ptr,8);
				Value *dataGEP = builder.CreateGEP(data_ptr,index_value);
				Value *data_ptr_32_8 = builder.CreateBitCast(dataGEP, type_32_8_pointer);

				dataGEP = builder.CreateGEP(type_32_8,data_ptr_32_8,builder.getInt32(0));
				Value *data_value = builder.CreateAlignedLoad(dataGEP,1);
				data_value = _mm256_permutevar8x32_epi32(gen,builder,data_value,permute_mask_value);
				data_value = builder.CreateBitCast(data_value,type_8_32);

				Value *shuffled_value = _mm256_shuffle_epi8(gen,builder,data_value,shuffle_mask_value);
				//result[pos] = unzigzag
				shuffled_value = builder.CreateBitCast(shuffled_value,type_32_8);
				Value *result_value = unZigZag32(gen,builder,shuffled_value,zero,one);
				Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
				Value *resultGEP = builder.CreateGEP(gen->int_type(32),result_ptr,pos_value);
				resultGEP = builder.CreateBitCast(resultGEP,type_32_8_pointer);
				builder.CreateAlignedStore(result_value,resultGEP,16);

				index_value = builder.CreateAdd(index_value,builder.getInt64(24));
				builder.CreateAlignedStore(index_value,index_ptr,8);
				pos_value = builder.CreateAdd(pos_value,builder.getInt64(8));
				builder.CreateAlignedStore(pos_value,pos_ptr,8);
				builder.CreateBr(label_for_cond);
			}
			builder.SetInsertPoint(label_for_end);
		}



		BasicBlock* label_for_cond = BasicBlock::Create(gen->context(), "for.cond",fn,0);
		BasicBlock* label_for_body = BasicBlock::Create(gen->context(), "for.body",fn,0);
		BasicBlock* label_for_inc = BasicBlock::Create(gen->context(), "for.inc",fn,0);

		builder.CreateBr(label_for_cond);
		builder.SetInsertPoint(label_for_cond);
		{
			//i<end;
			Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
			Value *cmpvalue = builder.CreateICmpULT(pos_value,end_value);
			builder.CreateCondBr(cmpvalue,label_for_body,label_end);
		}
		builder.SetInsertPoint(label_for_body);
		{
			//read certain number of bytes
			Value *datavalue[info->getFixedBitSize()/8];
			advance(gen,builder,data_ptr, index_ptr, datavalue,info->getFixedBitSize()/8);
			Value *rawresult_value = gen->getConstant(result_bitwidth,0);
			//read bytes
			for(int i=0;i<info->getFixedBitSize()/8;i++){
				Value *shiftvalue;
				if(i==0){
					shiftvalue = rawresult_value;
				}else{
					shiftvalue = builder.CreateShl(rawresult_value,gen->getConstant(result_bitwidth,8));
				}
				Value *datavalue_ext = builder.CreateSExt(datavalue[i],gen->int_type(result_bitwidth));
				Value *andvalue = builder.CreateAnd(datavalue_ext,gen->getConstant(result_bitwidth,0xff));
				rawresult_value = builder.CreateOr(shiftvalue,andvalue);
			}
			//if signed unzigzag
			if(issigned){
				rawresult_value = builder.CreateCall(genfunc_unZigZag(gen,result_bitwidth),rawresult_value);
			}

			Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
			Value *resultGEP = builder.CreateGEP(gen->int_type(result_bitwidth),result_ptr,pos_value);
			builder.CreateAlignedStore(rawresult_value,resultGEP,8);
			builder.CreateBr(label_for_inc);
		}
		//increment result offset by 1
		builder.SetInsertPoint(label_for_inc);
		{
			Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
			pos_value = builder.CreateAdd(pos_value,gen->getConstant(64,1));
			builder.CreateAlignedStore(pos_value,pos_ptr,8);
			builder.CreateBr(label_for_cond);
		}
	}else{

		BasicBlock* label_for_cond = BasicBlock::Create(gen->context(), "for.cond",fn,0);
		BasicBlock* label_for_body = BasicBlock::Create(gen->context(), "for.body",fn,0);
		BasicBlock* label_for_inc = BasicBlock::Create(gen->context(), "for.inc",fn,0);

		Value *rawresult_ptr = builder.CreateAlloca(gen->int_type(result_bitwidth));
		Value *bitsLeft_ptr = builder.CreateAlloca(gen->int_type(result_bitwidth));
		builder.CreateAlignedStore(gen->getConstant(result_bitwidth,0),bitsLeft_ptr,8);
		Value *bitsLeftToRead_ptr = builder.CreateAlloca(gen->int_type(result_bitwidth));
		Value *curByte_ptr = builder.CreateAlloca(gen->int_type(8));

		builder.CreateBr(label_for_cond);
		builder.SetInsertPoint(label_for_cond);
		{
			//i<end;
			Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
			Value *cmpvalue = builder.CreateICmpULT(pos_value,end_value);
			builder.CreateCondBr(cmpvalue,label_for_body,label_end);
		}

		builder.SetInsertPoint(label_for_body);
		{

			/*
			 * rawresult <<= 8;
				rawresult |= data[index++]&0xff;
			 * */
			BasicBlock* label_if_then = BasicBlock::Create(gen->context(), "if.then",fn,0);
			BasicBlock* label_if_end = BasicBlock::Create(gen->context(), "if.end",fn,0);
			BasicBlock* label_while_cond = BasicBlock::Create(gen->context(), "while.cond",fn,0);
			BasicBlock* label_while_body = BasicBlock::Create(gen->context(), "while.body",fn,0);
			BasicBlock* label_while_end = BasicBlock::Create(gen->context(), "while.end",fn,0);
			BasicBlock* label_if_then_1 = BasicBlock::Create(gen->context(), "if.then.1",fn,0);
			BasicBlock* label_if_end_1 = BasicBlock::Create(gen->context(), "if.end.1",fn,0);

			{
				//rawresult = 0;
				builder.CreateAlignedStore(gen->getConstant(result_bitwidth,0),rawresult_ptr,8);
				//bitsLeftToRead = fb;
				builder.CreateAlignedStore(bitwidth_value,bitsLeftToRead_ptr,8);
				//if(bitsLeft==0)
				Value *bitsLeft_value = builder.CreateAlignedLoad(bitsLeft_ptr,8);
				Value *cmpvalue = builder.CreateICmpEQ(bitsLeft_value,gen->getConstant(result_bitwidth,0));
				builder.CreateCondBr(cmpvalue,label_if_then,label_if_end);
			}
			{
				//curByte = data[index++];
				//bitsLeft = 8;
				builder.SetInsertPoint(label_if_then);
				Value *datavalue[1];
				advance(gen,builder,data_ptr, index_ptr, datavalue, 1);
				builder.CreateAlignedStore(datavalue[0],curByte_ptr,1);
				builder.CreateAlignedStore(gen->getConstant(result_bitwidth,8), bitsLeft_ptr,8);
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
				Value *curBytevalue_ext = builder.CreateSExt(curByte_value,gen->int_type(result_bitwidth));
				Value *shiftvalue1 = builder.CreateShl(gen->getConstant(result_bitwidth,1),bitsLeft_value);
				Value *subvalue = builder.CreateSub(shiftvalue1,gen->getConstant(result_bitwidth,1));
				Value *andvalue = builder.CreateAnd(curBytevalue_ext,subvalue);
				Value *orvalue = builder.CreateOr(shiftvalue,andvalue);
				builder.CreateAlignedStore(orvalue,rawresult_ptr,8);
				//bitsLeftToRead -= bitsLeft;
				Value *bitsLeftToRead_value = builder.CreateAlignedLoad(bitsLeftToRead_ptr,8);
				Value *subvalue1 = builder.CreateSub(bitsLeftToRead_value,bitsLeft_value);
				builder.CreateAlignedStore(subvalue1,bitsLeftToRead_ptr,8);
				//curByte = data[index++];
				//bitsLeft = 8;
				Value *datavalue[1];
				advance(gen,builder,data_ptr, index_ptr, datavalue, 1);
				builder.CreateAlignedStore(datavalue[0],curByte_ptr,8);
				builder.CreateAlignedStore(gen->getConstant(result_bitwidth,8),bitsLeft_ptr,8);
				builder.CreateBr(label_while_cond);
			}

			builder.SetInsertPoint(label_while_end);
			{
				//if(bitsLeftToRead > 0)
				Value *bitsLeftToRead_value = builder.CreateAlignedLoad(bitsLeftToRead_ptr,8);
				Value *cmpvalue = builder.CreateICmpUGT(bitsLeftToRead_value,gen->getConstant(result_bitwidth,0));
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
				Value *curBytevalue_ext = builder.CreateSExt(curByte_value,gen->int_type(result_bitwidth));
				Value *shiftvalue1 = builder.CreateAShr(curBytevalue_ext,subvalue);
				Value *shiftvalue2 = builder.CreateShl(gen->getConstant(result_bitwidth,1),bitsLeftToRead_value);
				Value *subvalue1 = builder.CreateSub(shiftvalue2,gen->getConstant(result_bitwidth,1));
				Value *andvalue = builder.CreateAnd(shiftvalue1,subvalue1);
				rawresult_value = builder.CreateOr(rawresult_value,andvalue);
				builder.CreateAlignedStore(rawresult_value,rawresult_ptr,8);
				builder.CreateBr(label_if_end_1);
			}
			builder.SetInsertPoint(label_if_end_1);
			{
				Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
				Value *rawresult_value = builder.CreateAlignedLoad(rawresult_ptr,8);
				if(issigned){
					rawresult_value = builder.CreateCall(genfunc_unZigZag(gen,result_bitwidth),rawresult_value);
				}
				Value *resultGEP = builder.CreateGEP(gen->int_type(result_bitwidth),result_ptr,pos_value);
				builder.CreateAlignedStore(rawresult_value,resultGEP,8);
				builder.CreateBr(label_for_inc);
			}
		}
		builder.SetInsertPoint(label_for_inc);
		{
			Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
			pos_value = builder.CreateAdd(pos_value,gen->getConstant(64,1));
			builder.CreateAlignedStore(pos_value,pos_ptr,8);
			builder.CreateBr(label_for_cond);
		}

	}

	builder.SetInsertPoint(label_end);
	builder.CreateRet(len_value);
	return fn;

}

}
