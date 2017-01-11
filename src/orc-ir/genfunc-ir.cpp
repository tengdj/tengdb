/*
 * RLE2-ir.cpp
 *
 *  Created on: Jul 9, 2016
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
#include <iostream>

#include "llvm-codegen.h"
#include "ColumnInfo.h"
#include "functions-ir.h"
#include "vectorization-ir.h"

using namespace llvm;

namespace orc{

//bool printencoding = true;
bool printdirect = false;
bool printrepeat = false;
bool printdelta = false;
bool printpatched = false;

Function *genfunc_nextDirect(LlvmCodeGen *gen, ColumnInfo *cinfo, bool isinline){


	EncodingInfo *info = cinfo->rleinfo->directInfo;
	const int result_bitwidth = orc::bitofType(cinfo->column.type);

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"next_"+cinfo->column.name+"_direct",IntegerType::getInt64Ty(gen->context()),isinline);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));
	proto.AddArgument("runlength", gen->int_type(64));
	proto.AddArgument("result", gen->int_ptr_type(8));


	llvm::Value *params[5];
	Function *fn = proto.GeneratePrototype(&builder,params);

	if(printdirect){
		print(gen,builder,"start direct: \n");
	}
	Value *data_ptr = params[0];
	Value *index_ptr = params[1];
	Value *offset_value = params[2];
	Value *space_ptr = params[3];
	Value *result_ptr = builder.CreateBitCast(params[4],gen->int_ptr_type(result_bitwidth));

	Value *firstbyte_value;
	Value *secondbyte_value;
	{
		Value *datavalue[2];
		advance(gen,builder,data_ptr,index_ptr,datavalue,2);
		firstbyte_value = datavalue[0];
		secondbyte_value = datavalue[1];
	}

	Value *bitWidth_value;
	//bitWidth = decodeBitWidth(((firstbyte >> 1) & 0x1f));
	{
		Value *shiftvalue = builder.CreateAShr(firstbyte_value,gen->getConstant(8,1));
		Value *andvalue = builder.CreateAnd(shiftvalue,gen->getConstant(8,0x1f));
		Value *andvalue_32 = builder.CreateSExt(andvalue,gen->int_type(32));
		//print(gen,builder,"%d\t",andvalue_32);
		bitWidth_value = getGlobalArray(gen,builder,"decodeBitWidth",andvalue_32);
		bitWidth_value = builder.CreateZExtOrTrunc(bitWidth_value,gen->int_type(result_bitwidth));
		//print(gen,builder,"%ld\n",bitWidth_value);

	}
	Value *runLength_value;
	//runLength = (((firstbyte&0x01)<<8) | secondbyte) + 1
	{
		Value *andvalue = builder.CreateAnd(firstbyte_value,gen->getConstant(8,1));
		Value *andvalue_64 = builder.CreateSExt(andvalue,gen->int_type(64));
		Value *shiftvalue = builder.CreateShl(andvalue_64,gen->getConstant(64,8));
		Value *secondbytevalue_64 = builder.CreateZExt(secondbyte_value,gen->int_type(64));
		Value *orvalue = builder.CreateOr(shiftvalue, secondbytevalue_64);
		runLength_value = builder.CreateAdd(orvalue,gen->getConstant(64,1));
	}

	std::vector<Value *> call_params;
	call_params.push_back(data_ptr);
	call_params.push_back(index_ptr);
	call_params.push_back(offset_value);
	call_params.push_back(runLength_value);
	call_params.push_back(bitWidth_value);
	call_params.push_back(result_ptr);

	Value *ret = builder.CreateCall(genfunc_readLongs(gen, info, result_bitwidth,info->issigned,true),call_params);
	if(printdirect){
		print(gen,builder,"end direct: %ld\n",ret);
	}
	builder.CreateRet(ret);
	if(!isinline)
		gen->AddFunctionToJit(fn,(void **)&fn);
	return fn;
}


Function *genfunc_nextRepeat(LlvmCodeGen *gen, ColumnInfo *cinfo, bool isinline){

	EncodingInfo *info = cinfo->rleinfo->repeatInfo;
	const int result_bitwidth = orc::bitofType(cinfo->column.type);

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"next_"+cinfo->column.name+"_repeat",IntegerType::getInt64Ty(gen->context()),isinline);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));
	proto.AddArgument("runlength", gen->int_type(64));
	proto.AddArgument("result", gen->int_ptr_type(8));


	llvm::Value *params[5];
	Function *fn = proto.GeneratePrototype(&builder,params);

	if(printrepeat){
		print(gen,builder,"start repeat: \n");
	}

	Value *data_ptr = params[0];
	Value *index_ptr = params[1];
	Value *offset_value = params[2];
	Value *space_ptr = params[3];
	Value *result_ptr = builder.CreateBitCast(params[4],gen->int_ptr_type(result_bitwidth));

	//firstByte = data[index++];
	Value *firstbyte_value;
	{
		Value *datavalue[1];
		advance(gen,builder,data_ptr, index_ptr, datavalue,1);
		firstbyte_value = datavalue[0];
	}
	Value *value_value;
	{
		//byteSize = (firstByte >> 3) & 0x07 + 1;
		//value = readLongBE(data, index, byteSize);
		Value *bytesize;
		if(info->isBitSizeFixed()){
			bytesize = gen->getConstant(result_bitwidth,info->getFixedBitSize());;
		}else{
			Value *shiftvalue = builder.CreateAShr(firstbyte_value,gen->getConstant(8,3));
			Value *andvalue = builder.CreateAnd(shiftvalue,gen->getConstant(8,0x07));
			Value *andvalue_ext = builder.CreateSExt(andvalue,gen->int_type(result_bitwidth));
			bytesize = builder.CreateAdd(andvalue_ext,gen->getConstant(result_bitwidth,1));
		}

		std::vector<Value *> call_params;
		call_params.push_back(data_ptr);
		call_params.push_back(index_ptr);
		call_params.push_back(bytesize);
		value_value = builder.CreateCall(genfunc_readLongBE(gen,info,result_bitwidth),call_params);
		//value = unZigZag(value); for signed
		if(info->issigned){
			value_value = builder.CreateCall(genfunc_unZigZag(gen,result_bitwidth),value_value);
		}
	}
	if(printrepeat){
		print(gen,builder,"value: %d\n",value_value);
	}

	Value *runLength_value;
	{
		//runLength = firstByte & 0x07 + 3;
		Value *andvalue = builder.CreateAnd(firstbyte_value,gen->getConstant(8,0x07));
		Value *andvalue_64 = builder.CreateSExt(andvalue,gen->int_type(64));
		runLength_value = builder.CreateAdd(andvalue_64,gen->getConstant(64,3));//MIN_REPEAT
	}

	Value *pos_ptr = builder.CreateAlloca(gen->int_type(64));
	Value *end_value;
	{
		//pos = offset
		//end = runLength + offset;
		end_value = builder.CreateAdd(offset_value, runLength_value);
		builder.CreateAlignedStore(offset_value,pos_ptr,8);
	}

	BasicBlock *label_for_cond = BasicBlock::Create(gen->context(),"for.cond",fn);
	BasicBlock *label_for_body = BasicBlock::Create(gen->context(),"for.body",fn);
	BasicBlock *label_for_end = BasicBlock::Create(gen->context(),"for.end",fn);
	BasicBlock *label_for_inc = BasicBlock::Create(gen->context(),"for.inc",fn);

	builder.CreateBr(label_for_cond);
	builder.SetInsertPoint(label_for_cond);
	{
		//pos < end
		Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
		Value *cmpvalue = builder.CreateICmpULT(pos_value,end_value);
		builder.CreateCondBr(cmpvalue,label_for_body,label_for_end);
	}

	builder.SetInsertPoint(label_for_body);
	{
		//result[pos] = value;
		Value *pos_value = builder.CreateAlignedLoad(pos_ptr,64);
		Value *resultGEP = builder.CreateGEP(gen->int_type(result_bitwidth),result_ptr,pos_value);
		builder.CreateAlignedStore(value_value,resultGEP,8);
		builder.CreateBr(label_for_inc);
	}

	builder.SetInsertPoint(label_for_inc);
	{
		//pos++;
		Value *pos_value = builder.CreateAlignedLoad(pos_ptr,64);
		Value *incvalue = builder.CreateAdd(pos_value,gen->getConstant(64,1));
		builder.CreateAlignedStore(incvalue,pos_ptr,8);
		builder.CreateBr(label_for_cond);
	}

	builder.SetInsertPoint(label_for_end);
	if(printrepeat)
	{
		print(gen,builder,"end repeat: %ld\n",runLength_value);
	}

	builder.CreateRet(runLength_value);
	if(!isinline)
		gen->AddFunctionToJit(fn,(void **)fn);
	return fn;
}


Function *genfunc_nextDelta(LlvmCodeGen *gen,ColumnInfo *cinfo, bool isinline){

	EncodingInfo *info = cinfo->rleinfo->deltaInfo;
	const int result_bitwidth = orc::bitofType(cinfo->column.type);

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"next_"+cinfo->column.name+"_delta",IntegerType::getInt64Ty(gen->context()),isinline);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));
	proto.AddArgument("runlength", gen->int_type(64));
	proto.AddArgument("result", gen->int_ptr_type(8));


	llvm::Value *params[5];
	Function *fn = proto.GeneratePrototype(&builder,params);

	if(printdelta){
		print(gen,builder,"\nstart delta: \n");
	}
	Value *data_ptr = params[0];
	Value *index_ptr = params[1];
	Value *offset_value = params[2];
	Value *space_ptr = params[3];
	Value *result_ptr = builder.CreateBitCast(params[4],gen->int_ptr_type(result_bitwidth));


	Value *firstbyte_value;
	Value *secondbyte_value;
	//firstbyte = data[index++];
	{
		Value *datavalue[2];
		advance(gen,builder,data_ptr, index_ptr, datavalue,2);
		firstbyte_value = datavalue[0];
		secondbyte_value = datavalue[1];


	}


	Value *bitSize_ptr = builder.CreateAlloca(gen->int_type(result_bitwidth));
	builder.CreateAlignedStore(gen->getConstant(result_bitwidth,0),bitSize_ptr,8);

	//if((!info->isBitSizeFixed()||info->getFixedBitSize()!=0))
	{//
		BasicBlock *label_if_cond_fbo = BasicBlock::Create(gen->context(),"if.cond.fbo",fn);
		BasicBlock *label_if_then_fbo = BasicBlock::Create(gen->context(),"if.then.fbo",fn);
		BasicBlock *label_if_end_fbo = BasicBlock::Create(gen->context(),"if.end.fbo",fn);

		Value *fbo_value;
		//fbo = (firstbyte >> 1)&0x1f
		{
			Value *shiftvalue = builder.CreateAShr(firstbyte_value,gen->getConstant(8,1));
			Value *andvalue = builder.CreateAnd(shiftvalue,gen->getConstant(8,0x1f));
			fbo_value = builder.CreateZExtOrTrunc(andvalue,gen->int_type(result_bitwidth));

		}

		builder.CreateBr(label_if_cond_fbo);
		builder.SetInsertPoint(label_if_cond_fbo);
		//if(fbo != 0)
		{
			Value *cmp_value = builder.CreateICmpNE(fbo_value,gen->getConstant(result_bitwidth,0));
			builder.CreateCondBr(cmp_value,label_if_then_fbo,label_if_end_fbo);
		}
		//bitSize = decodeBitWidth(fbo);
		builder.SetInsertPoint(label_if_then_fbo);
		{

			Value *bitSize_value = getGlobalArray(gen,builder,"decodeBitWidth",fbo_value);
			bitSize_value = builder.CreateZExtOrTrunc(bitSize_value,gen->int_type(result_bitwidth));
			builder.CreateAlignedStore(bitSize_value,bitSize_ptr,8);
			builder.CreateBr(label_if_end_fbo);
		}
		builder.SetInsertPoint(label_if_end_fbo);

	}


	Value *runLength_value;
	//runLength = ((firstbyte & 0x01) << 8) | secondbyte + 1
	{
		Value *andvalue = builder.CreateAnd(firstbyte_value,gen->getConstant(8,0x01));
		Value *andvalue_64 = builder.CreateSExt(andvalue,gen->int_type(64));
		Value *shiftvalue = builder.CreateShl(andvalue_64, gen->getConstant(64,8));
		Value *secondbyte_value_64 = builder.CreateZExt(secondbyte_value, gen->int_type(64));
		Value *orvalue = builder.CreateOr(shiftvalue,secondbyte_value_64);
		runLength_value = builder.CreateAdd(orvalue,gen->getConstant(64,1));
	}

	//firstvalue = readVLong(data,index,sign);
	std::vector<Value *> call_params;
	call_params.push_back(data_ptr);
	call_params.push_back(index_ptr);
	Value *firstValue_value = builder.CreateCall(genfunc_readVLong(gen,result_bitwidth,info->issigned),call_params);
	//deltabase = readVsLong(data,index);
	Value *deltaBase_value = builder.CreateCall(genfunc_readVLong(gen,result_bitwidth,true),call_params);
	//pos = offset;
	//end = offset + runLength;
	//prevValue = firstValue;
	Value *pos_ptr = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(offset_value,pos_ptr,8);
	Value *end_value = builder.CreateAdd(offset_value,runLength_value);
	Value *prevValue_ptr = builder.CreateAlloca(gen->int_type(result_bitwidth));
	builder.CreateAlignedStore(firstValue_value,prevValue_ptr,8);

	//result[pos++] = firstValue;
	{
		Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
		Value *incvalue = builder.CreateAdd(pos_value,gen->getConstant(64,1));
		builder.CreateAlignedStore(incvalue,pos_ptr,8);
		Value *resultGEP = builder.CreateGEP(gen->int_type(result_bitwidth),result_ptr,pos_value);
		builder.CreateAlignedStore(firstValue_value,resultGEP,8);
		if(printdelta){
			print(gen,builder,"first value: %d\n",firstValue_value);
		}
	}


	BasicBlock *label_if_end_bitsize = BasicBlock::Create(gen->context(),"if.end.bitsize",fn);


//	if(false &&info->getFixedBitSize()==0&&info->getMinRunlength()>4){
//		{
//			Value *prevValue_value = builder.CreateAlignedLoad(prevValue_ptr,8);
//			Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
//			for(int i=0;i<info->getMinRunlength()-2;i++){
//				prevValue_value = builder.CreateAdd(prevValue_value,deltaBase_value);
//				Value *resultGEP = builder.CreateGEP(gen->int_type(result_bitwidth),result_ptr,pos_value);
//				pos_value = builder.CreateAdd(pos_value,gen->getConstant(64,1));
//				builder.CreateAlignedStore(prevValue_value,resultGEP,8);
//			}
//			builder.CreateAlignedStore(prevValue_value,prevValue_ptr,8);
//			builder.CreateAlignedStore(pos_value,pos_ptr,8);
//		}
//
//		BasicBlock *label_for_cond_bitsize = BasicBlock::Create(gen->context(),"for.cond.bitsize",fn);
//		BasicBlock *label_for_body_bitsize = BasicBlock::Create(gen->context(),"for.body.bitsize",fn);
//		BasicBlock *label_for_inc_bitsize = BasicBlock::Create(gen->context(),"for.inc.bitsize",fn);
//
//		builder.CreateBr(label_for_cond_bitsize);
//		//for(;pos<end;)
//		builder.SetInsertPoint(label_for_cond_bitsize);
//		{
//			Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
//			Value *cmp_value = builder.CreateICmpULT(pos_value,end_value);
//			builder.CreateCondBr(cmp_value,label_for_body_bitsize,label_if_end_bitsize);
//		}
//		//prevValue = result[pos] = prevValue + deltabase;
//		builder.SetInsertPoint(label_for_body_bitsize);
//		{
//			Value *prevValue_value = builder.CreateAlignedLoad(prevValue_ptr,8);
//			Value *addvalue = builder.CreateAdd(prevValue_value,deltaBase_value);
//			Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
//			Value *resultGEP = builder.CreateGEP(gen->int_type(result_bitwidth),result_ptr,pos_value);
//			builder.CreateAlignedStore(addvalue,resultGEP,8);
//			builder.CreateAlignedStore(addvalue,prevValue_ptr,8);
//			builder.CreateBr(label_for_inc_bitsize);
//		}
//		//pos++
//		builder.SetInsertPoint(label_for_inc_bitsize);
//		{
//			Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
//			Value *incvalue = builder.CreateAdd(pos_value,gen->getConstant(64,1));
//			builder.CreateAlignedStore(incvalue,pos_ptr,8);
//			builder.CreateBr(label_for_cond_bitsize);
//		}
//	}else
	{
		BasicBlock *label_if_cond_bitsize = BasicBlock::Create(gen->context(),"if.cond.bitsize",fn);
		BasicBlock *label_if_then_bitsize = BasicBlock::Create(gen->context(),"if.then.bitsize",fn);
		BasicBlock *label_if_else_bitsize = BasicBlock::Create(gen->context(),"if.else.bitsize",fn);
		builder.CreateBr(label_if_cond_bitsize);
		builder.SetInsertPoint(label_if_cond_bitsize);
		//if(bitSize == 0)
		{
			Value *bitSize_value = builder.CreateAlignedLoad(bitSize_ptr,8);

			Value *cmp_value = builder.CreateICmpEQ(bitSize_value,gen->getConstant(result_bitwidth,0));
			builder.CreateCondBr(cmp_value,label_if_then_bitsize,label_if_else_bitsize);
		}
		builder.SetInsertPoint(label_if_then_bitsize);
		{
			BasicBlock *label_for_cond_bitsize = BasicBlock::Create(gen->context(),"for.cond.bitsize",fn);
			BasicBlock *label_for_body_bitsize = BasicBlock::Create(gen->context(),"for.body.bitsize",fn);
			BasicBlock *label_for_inc_bitsize = BasicBlock::Create(gen->context(),"for.inc.bitsize",fn);

			builder.CreateBr(label_for_cond_bitsize);
			//for(;pos<end;)
			builder.SetInsertPoint(label_for_cond_bitsize);
			{
				Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
				Value *cmp_value = builder.CreateICmpULT(pos_value,end_value);
				builder.CreateCondBr(cmp_value,label_for_body_bitsize,label_if_end_bitsize);
			}
			//prevValue = result[pos] = prevValue + deltabase;
			builder.SetInsertPoint(label_for_body_bitsize);
			{
				Value *prevValue_value = builder.CreateAlignedLoad(prevValue_ptr,8);
				Value *addvalue = builder.CreateAdd(prevValue_value,deltaBase_value);
				Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
				Value *resultGEP = builder.CreateGEP(gen->int_type(result_bitwidth),result_ptr,pos_value);
				if(printdelta){
					print(gen,builder,"then value: %d\n",addvalue);
				}
				builder.CreateAlignedStore(addvalue,resultGEP,8);
				builder.CreateAlignedStore(addvalue,prevValue_ptr,8);
				builder.CreateBr(label_for_inc_bitsize);
			}
			//pos++
			builder.SetInsertPoint(label_for_inc_bitsize);
			{
				Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
				Value *incvalue = builder.CreateAdd(pos_value,gen->getConstant(64,1));
				builder.CreateAlignedStore(incvalue,pos_ptr,8);
				builder.CreateBr(label_for_cond_bitsize);
			}
		}//end if bitsize==0

		builder.SetInsertPoint(label_if_else_bitsize);
		{

			BasicBlock *label_if_cond_else = BasicBlock::Create(gen->context(),"if.cond.else",fn);
			BasicBlock *label_if_then_else = BasicBlock::Create(gen->context(),"if.then.else",fn);
			BasicBlock *label_if_end_else = BasicBlock::Create(gen->context(),"if.end.else",fn);

			builder.CreateBr(label_if_cond_else);
			builder.SetInsertPoint(label_if_cond_else);
			//if(pos<end)
			{
				Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
				Value *cmp_value = builder.CreateICmpULT(pos_value,end_value);
				builder.CreateCondBr(cmp_value,label_if_then_else,label_if_end_else);
			}
			//prevValue = result[pos++] = firstValue + deltaBase;
			builder.SetInsertPoint(label_if_then_else);
			{
				Value *addvalue = builder.CreateAdd(firstValue_value,deltaBase_value);
				Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
				Value *incvalue = builder.CreateAdd(pos_value,gen->getConstant(64,1));
				builder.CreateAlignedStore(incvalue,pos_ptr,8);

				Value *resultGEP = builder.CreateGEP(gen->int_type(result_bitwidth),result_ptr,pos_value);
				if(printdelta){
					print(gen,builder,"second value: %d\n",addvalue);
				}
				builder.CreateAlignedStore(addvalue,resultGEP,8);
				builder.CreateAlignedStore(addvalue,prevValue_ptr,8);
				builder.CreateBr(label_if_end_else);
			}
			builder.SetInsertPoint(label_if_end_else);
			//remaining = end - pos;
			//readLongs(data, result, index, pos, remaining, bitsize);
			Value *bitSize_value = builder.CreateAlignedLoad(bitSize_ptr,8);
			Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
			Value *remaining_value = builder.CreateSub(end_value,pos_value);
			std::vector<Value *> call_params;
			call_params.push_back(data_ptr);
			call_params.push_back(index_ptr);
			call_params.push_back(pos_value);
			call_params.push_back(remaining_value);
			call_params.push_back(bitSize_value);
			call_params.push_back(result_ptr);
			builder.CreateCall(genfunc_readLongs(gen,info,result_bitwidth,false,true),call_params);

			BasicBlock *label_if_cond_deltabase = BasicBlock::Create(gen->context(),"if.cond.deltabase",fn);
			BasicBlock *label_if_then_deltabase = BasicBlock::Create(gen->context(),"if.then.deltabase",fn);
			BasicBlock *label_if_else_deltabase = BasicBlock::Create(gen->context(),"if.else.deltabase",fn);
			builder.CreateBr(label_if_cond_deltabase);
			//if(deltabase < 0)
			builder.SetInsertPoint(label_if_cond_deltabase);
			{
				Value *cmp_value = builder.CreateICmpSLT(deltaBase_value,gen->getConstant(result_bitwidth,0));
				builder.CreateCondBr(cmp_value,label_if_then_deltabase,label_if_else_deltabase);
			}
			for(int i=0;i<2;i++){
				if(i==0){
					builder.SetInsertPoint(label_if_then_deltabase);
				}else{
					builder.SetInsertPoint(label_if_else_deltabase);
				}

				{
					BasicBlock *label_for_cond_then = BasicBlock::Create(gen->context(),"for.cond.then",fn);
					BasicBlock *label_for_body_then = BasicBlock::Create(gen->context(),"for.body.then",fn);
					BasicBlock *label_for_inc_then = BasicBlock::Create(gen->context(),"for.inc.then",fn);

					builder.CreateBr(label_for_cond_then);
					builder.SetInsertPoint(label_for_cond_then);
					//for(;pos<end;pos++)
					{
						Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
						Value *cmp_value = builder.CreateICmpULT(pos_value,end_value);
						builder.CreateCondBr(cmp_value,label_for_body_then,label_if_end_bitsize);
					}
					builder.SetInsertPoint(label_for_body_then);
					//prevValue = result[pos] = prevValue - result[pos];
					//prevValue = result[pos] = prevValue + result[pos];

					{
						Value *prevValue_value = builder.CreateAlignedLoad(prevValue_ptr,8);
						Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
						Value *resultGEP = builder.CreateGEP(gen->int_type(result_bitwidth),result_ptr,pos_value);
						Value *result_value = builder.CreateAlignedLoad(resultGEP,8);
						if(i==0){
							result_value = builder.CreateSub(prevValue_value,result_value);
						}else{
							result_value = builder.CreateAdd(prevValue_value,result_value);
						}
						builder.CreateAlignedStore(result_value,resultGEP,8);
						builder.CreateAlignedStore(result_value,prevValue_ptr,8);
						builder.CreateBr(label_for_inc_then);
					}
					builder.SetInsertPoint(label_for_inc_then);
					//pos++;
					{
						Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
						Value *incvalue = builder.CreateAdd(pos_value,gen->getConstant(64,1));
						builder.CreateAlignedStore(incvalue,pos_ptr,8);
						builder.CreateBr(label_for_cond_then);
					}
				}
			}
		}//if bitsize!=0
	}


	builder.SetInsertPoint(label_if_end_bitsize);

	if(printdelta)
	{
		print(gen,builder,"end delta: %ld\n",runLength_value);
	}

	builder.CreateRet(runLength_value);
	if(!isinline)
		gen->AddFunctionToJit(fn,(void **)fn);
	return fn;

}



Function *genfunc_nextPatched(LlvmCodeGen *gen, ColumnInfo *cinfo,bool isinline){


	EncodingInfo *info = cinfo->rleinfo->patchedInfo;
	const int result_bitwidth = orc::bitofType(cinfo->column.type);

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"next_"+cinfo->column.name+"_patched",IntegerType::getInt64Ty(gen->context()),isinline);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));
	proto.AddArgument("runlength", gen->int_type(64));
	proto.AddArgument("result", gen->int_ptr_type(8));


	llvm::Value *params[5];
	Function *fn = proto.GeneratePrototype(&builder,params);

	if(printpatched){
		print(gen,builder,"start patched: \n");
	}
	Value *data_ptr = params[0];
	Value *index_ptr = params[1];
	Value *offset_value = params[2];
	Value *space_ptr = params[3];
	Value *result_ptr = builder.CreateBitCast(params[4],gen->int_ptr_type(result_bitwidth));


	Value *firstbyte_value;
	Value *secondbyte_value;
	Value *thirdbyte_value;
	Value *fourthbyte_value;
	//firstbyte = data[index++];
	//secondbyte = data[index++];
	//thirdbyte = data[index++];
	//fourthbyte = data[index++];

	{
		Value *datavalue[4];
		advance(gen,builder,data_ptr,index_ptr,datavalue,4);
		firstbyte_value = datavalue[0];
		secondbyte_value = datavalue[1];
		thirdbyte_value = datavalue[2];
		fourthbyte_value = datavalue[3];
	}

	Value *bitWidth_value;
	//bitWidth = decodeBitWidth(((firstbyte >> 1) & 0x1f));
	{
		Value *shiftvalue = builder.CreateAShr(firstbyte_value,gen->getConstant(8,1));
		Value *andvalue = builder.CreateAnd(shiftvalue,gen->getConstant(8,0x1f));
		Value *andvalue_32 = builder.CreateZExt(andvalue,gen->int_type(32));
		bitWidth_value = getGlobalArray(gen,builder,"decodeBitWidth",andvalue_32);
		bitWidth_value = builder.CreateZExtOrTrunc(bitWidth_value,gen->int_type(result_bitwidth));
		print(gen,builder,"bitWidth %d\n",bitWidth_value);

	}
	Value *runLength_value;
	//runLength = (((firstbyte&0x01)<<8) | secondbyte) + 1
	{
		Value *andvalue = builder.CreateAnd(firstbyte_value,gen->getConstant(8,1));
		Value *andvalue_64 = builder.CreateZExt(andvalue,gen->int_type(64));
		Value *shiftvalue = builder.CreateShl(andvalue_64,gen->getConstant(64,8));
		Value *secondbytevalue_64 = builder.CreateZExt(secondbyte_value,gen->int_type(64));
		Value *orvalue = builder.CreateOr(shiftvalue, secondbytevalue_64);
		runLength_value = builder.CreateAdd(orvalue,gen->getConstant(64,1));
	}

	Value *base_byteSize_value;
	//byteSize = (thirdbyte >> 5) & 0x07+1
	{
		Value *shiftvalue = builder.CreateAShr(thirdbyte_value,gen->getConstant(8,5));
		Value *andvalue = builder.CreateAnd(shiftvalue, gen->getConstant(8,0x07));
		Value *addvalue = builder.CreateAdd(andvalue,gen->getConstant(8,1));
		//byteSize_value = builder.CreateZExt(addvalue,gen->int_type(result_bitwidth));
		base_byteSize_value = builder.CreateZExtOrTrunc(addvalue,gen->int_type(result_bitwidth));
		print(gen,builder,"basebytesize %d\n",base_byteSize_value);

	}

	Value *patchBitSize_value;
	//patchBitSize = decodeWidth(thirdbyte & 0x1f);
	{
		Value *andvalue = builder.CreateAnd(thirdbyte_value,gen->getConstant(8,0x1f));
		Value *andvalue_32 = builder.CreateZExt(andvalue,gen->int_type(32));
		patchBitSize_value = getGlobalArray(gen,builder,"decodeBitWidth",andvalue_32);
		patchBitSize_value = builder.CreateZExtOrTrunc(patchBitSize_value,gen->int_type(result_bitwidth));
		//patchBitSize_value = builder.CreateZExt(patchBitSize_value,gen->int_type(64));
		print(gen,builder,"patchBitSize %d\n",patchBitSize_value);

	}

	Value *patchGapWidth_value;
	//patchGapWidth = (fourthbyte >> 5) & 0x07 + 1;
	{
		Value *shiftvalue = builder.CreateAShr(fourthbyte_value,gen->getConstant(8,5));
		Value *andvalue = builder.CreateAnd(shiftvalue,gen->getConstant(8,0x07));
		Value *addvalue = builder.CreateAdd(andvalue,gen->getConstant(8,1));
		patchGapWidth_value = builder.CreateZExtOrTrunc(addvalue,gen->int_type(result_bitwidth));
		//patchGapWidth_value = builder.CreateZExt(addvalue,gen->int_type(64));
	}

	Value *patchListLength_value;
	//patchListLength = fourthbyte & 0x1f;
	{
		Value *andvalue = builder.CreateAnd(fourthbyte_value,gen->getConstant(8,0x1f));
		patchListLength_value = builder.CreateZExt(andvalue,gen->int_type(64));
	}

	Value *base_ptr = builder.CreateAlloca(gen->int_type(result_bitwidth));
	//base = readLongBE(data, index, byteSize);
	{
		std::vector<Value *> callparams;
		callparams.push_back(data_ptr);
		callparams.push_back(index_ptr);
		callparams.push_back(base_byteSize_value);
		//callparams.push_back(gen->getConstant(64,2));

		Value *base_value = builder.CreateCall(orc::genfunc_readLongBE(gen,info,result_bitwidth),callparams);
		builder.CreateAlignedStore(base_value,base_ptr,8);
		print(gen,builder,"basevalue %d\n",base_value);


	}

	Value *mask_ptr = builder.CreateAlloca(gen->int_type(result_bitwidth));
	//mask = 1 << ((byteSize*8)-1);
	{
		Value *mulvalue = builder.CreateMul(base_byteSize_value,gen->getConstant(result_bitwidth,8));
		Value *subvalue = builder.CreateSub(mulvalue,gen->getConstant(result_bitwidth,1));
		Value *mask_value = builder.CreateShl(gen->getConstant(result_bitwidth,1),subvalue);
		builder.CreateAlignedStore(mask_value,mask_ptr,8);
	}

	BasicBlock *label_if_then = BasicBlock::Create(gen->context(),"label_if_then",fn);
	BasicBlock *label_if_end = BasicBlock::Create(gen->context(),"label_if_end",fn);
	{
		Value *base_value = builder.CreateAlignedLoad(base_ptr,8);
		Value *mask_value = builder.CreateAlignedLoad(mask_ptr,8);
		Value *andvalue = builder.CreateAnd(base_value,mask_value);
		Value *condvalue = builder.CreateICmpNE(andvalue,gen->getConstant(result_bitwidth,0));
		builder.CreateCondBr(condvalue,label_if_then,label_if_end);
		builder.SetInsertPoint(label_if_then);
		{
			Value *xorvalue = builder.CreateXor(mask_value,gen->getConstant(result_bitwidth,-1));
			Value *andvalue = builder.CreateAnd(base_value,xorvalue);
			Value *subvalue = builder.CreateSub(gen->getConstant(result_bitwidth,0),andvalue);
			builder.CreateAlignedStore(subvalue,base_ptr,8);
			builder.CreateBr(label_if_end);
		}
	}
	builder.SetInsertPoint(label_if_end);

	Value *unpacked_ptr = builder.CreateAlloca(gen->int_type(result_bitwidth),runLength_value);
	Value *unpackedPatch_ptr = builder.CreateAlloca(gen->int_type(result_bitwidth),patchListLength_value);

	{
		std::vector<Value *> callparams;
		callparams.push_back(data_ptr);
		callparams.push_back(index_ptr);
		callparams.push_back(gen->getConstant(64,0));
		callparams.push_back(runLength_value);
		callparams.push_back(bitWidth_value);
		callparams.push_back(unpacked_ptr);
		builder.CreateCall(genfunc_readLongs(gen,info,result_bitwidth,false,true),callparams);
	}
	Value *cfb_value;
	{
		Value *addvalue = builder.CreateAdd(patchBitSize_value,patchGapWidth_value);
		cfb_value = getGlobalArray(gen,builder,"closestFixedBits",addvalue);
		cfb_value = builder.CreateZExtOrTrunc(cfb_value,gen->int_type(result_bitwidth));

	}
	{
		std::vector<Value *> callparams;
		callparams.push_back(data_ptr);
		callparams.push_back(index_ptr);
		callparams.push_back(gen->getConstant(64,0));
		callparams.push_back(patchListLength_value);
		callparams.push_back(cfb_value);
		callparams.push_back(unpackedPatch_ptr);
		builder.CreateCall(genfunc_readLongs(gen,info,result_bitwidth,false,false),callparams);
	}

	BasicBlock *label_for_cond = BasicBlock::Create(gen->context(),"lable_for_cond",fn);
	BasicBlock *label_for_body = BasicBlock::Create(gen->context(),"lable_for_body",fn);
	BasicBlock *label_for_end = BasicBlock::Create(gen->context(),"lable_for_end",fn);

	Value *pos_ptr = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(offset_value,pos_ptr,8);

	Value *up_value = builder.CreateAdd(offset_value,runLength_value);
	Value *base_value = builder.CreateAlignedLoad(base_ptr,8);



	Value *unpackedIdx_ptr = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(gen->getConstant(64,0),unpackedIdx_ptr,8);

	builder.CreateBr(label_for_cond);
	builder.SetInsertPoint(label_for_cond);
	{
		Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
		Value *cond_value = builder.CreateICmpULT(pos_value,up_value);
		builder.CreateCondBr(cond_value,label_for_body,label_for_end);
	}

	builder.SetInsertPoint(label_for_body);
	{
		Value *unpackedIdx_value  = builder.CreateAlignedLoad(unpackedIdx_ptr,8);
		Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
		Value *unpackedGEP = builder.CreateGEP(gen->int_type(result_bitwidth),unpacked_ptr, unpackedIdx_value);
		Value *unpacked_value = builder.CreateAlignedLoad(unpackedGEP,8);
		Value *addvalue = builder.CreateAdd(unpacked_value,base_value);

		Value *resultGEP = builder.CreateGEP(gen->int_type(result_bitwidth),result_ptr, pos_value);
		builder.CreateAlignedStore(addvalue,resultGEP,8);


		Value *pos_addvalue = builder.CreateAdd(pos_value,gen->getConstant(64,1));
		builder.CreateAlignedStore(pos_addvalue,pos_ptr,8);

		Value *unpackedIdx_addvalue = builder.CreateAdd(unpackedIdx_value,gen->getConstant(64,1));
		builder.CreateAlignedStore(unpackedIdx_addvalue,unpackedIdx_ptr,8);
		builder.CreateBr(label_for_cond);
	}

	builder.SetInsertPoint(label_for_end);
	Value *patchMask_value;
	{
		patchMask_value = builder.CreateShl(gen->getConstant(result_bitwidth,1),patchBitSize_value);
		patchMask_value = builder.CreateSub(patchMask_value,gen->getConstant(result_bitwidth,1));
	}


	BasicBlock *label_patch_for_cond = BasicBlock::Create(gen->context(),"lable_patch_for_cond",fn);
	BasicBlock *label_patch_for_body = BasicBlock::Create(gen->context(),"lable_patch_for_body",fn);
	BasicBlock *label_patch_for_end = BasicBlock::Create(gen->context(),"lable_patch_for_end",fn);

	Value *patchIdx_ptr = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(gen->getConstant(64,0),patchIdx_ptr,8);
	builder.CreateAlignedStore(offset_value,pos_ptr,8);

	builder.CreateBr(label_patch_for_cond);
	builder.SetInsertPoint(label_patch_for_cond);
	Value *patchIdx_value;
	Value *pos_value;
	{
		patchIdx_value = builder.CreateAlignedLoad(patchIdx_ptr,8);
		Value *condvalue = builder.CreateICmpULT(patchIdx_value,patchListLength_value);

		builder.CreateCondBr(condvalue,label_patch_for_body,label_patch_for_end);
	}

	Value *curGap_value;

	Value *unpackedPatch_value;
	builder.SetInsertPoint(label_patch_for_body);
	{
		Value *patchIdx_value = builder.CreateAlignedLoad(patchIdx_ptr,8);
		Value *unpackedPatchGEP = builder.CreateGEP(gen->int_type(result_bitwidth),unpackedPatch_ptr,patchIdx_value);
		unpackedPatch_value = builder.CreateAlignedLoad(unpackedPatchGEP,8);
		curGap_value = builder.CreateAShr(unpackedPatch_value,patchBitSize_value);
		Value *curPatch_value = builder.CreateAnd(unpackedPatch_value,patchMask_value);
		pos_value = builder.CreateAlignedLoad(pos_ptr,8);
		pos_value = builder.CreateAdd(builder.CreateZExt(curGap_value,gen->int_type(64)),pos_value);
		builder.CreateAlignedStore(pos_value,pos_ptr,8);

		Value *resultGEP = builder.CreateGEP(gen->int_type(result_bitwidth),result_ptr,pos_value);
		Value *result_value = builder.CreateAlignedLoad(resultGEP,8);
		result_value = builder.CreateSub(result_value,base_value);
		Value *shiftvalue = builder.CreateShl(curPatch_value,bitWidth_value);
		result_value = builder.CreateOr(shiftvalue,result_value);
		result_value = builder.CreateAdd(result_value,base_value);
		builder.CreateAlignedStore(result_value,resultGEP,8);
		patchIdx_value = builder.CreateAdd(patchIdx_value,gen->getConstant(64,1));
		builder.CreateAlignedStore(patchIdx_value,patchIdx_ptr,8);
		builder.CreateBr(label_patch_for_cond);
	}

	builder.SetInsertPoint(label_patch_for_end);
	if(printpatched){
		print(gen,builder,"end patched: %d\n",runLength_value);
	}
	builder.CreateRet(runLength_value);
	if(!isinline)
		gen->AddFunctionToJit(fn,(void **)&fn);
	return fn;
}


Function *genfunc_rle(LlvmCodeGen *gen,ColumnInfo *cinfo){

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"next_"+cinfo->column.name,IntegerType::getInt64Ty(gen->context()),false);

	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));
	proto.AddArgument("runlength", gen->int_type(64));
	proto.AddArgument("result", gen->int_ptr_type(8));


	llvm::Value *params[5];
	Function *fn = proto.GeneratePrototype(&builder,params);

	RLEInfo *info = cinfo->rleinfo;

	int encoding_count = info->hasRepeat()+ info->hasDirect() + info->hasDelta() + info->hasPatched();
	assert(encoding_count>0);
	if(encoding_count==1){
		Function *nextfunc;
		if(info->hasRepeat()){
			nextfunc = genfunc_nextRepeat(gen, cinfo,true);
		}else if(info->hasDirect()){
			nextfunc = genfunc_nextDirect(gen, cinfo,true);
		}else if(info->hasDelta()){
			nextfunc = genfunc_nextDelta(gen,cinfo,true);
		}else if(info->hasPatched()){
			nextfunc = genfunc_nextPatched(gen,cinfo,true);
		}
		Value *ret = builder.CreateCall(nextfunc,params);
		builder.CreateRet(ret);
		gen->AddFunctionToJit(fn,(void**)&fn);
		return fn;
	}

	Function *nextFuncs[encoding_count];
	int funcEncoding[encoding_count];
	int count = 0;
	if(info->hasRepeat()){
		funcEncoding[count] = 0;
		nextFuncs[count] = genfunc_nextRepeat(gen, cinfo,true);
		count++;
	}
	if(info->hasDirect()){
		funcEncoding[count] = 1;
		nextFuncs[count] = genfunc_nextDirect(gen, cinfo,true);
		count++;
	}
	if(info->hasPatched()){
		funcEncoding[count] = 2;
		nextFuncs[count] = genfunc_nextPatched(gen, cinfo,true);
		count++;
	}
	if(info->hasDelta()){
		funcEncoding[count] = 3;
		nextFuncs[count] = genfunc_nextDelta(gen, cinfo,true);
	}


	Value *data_ptr = params[0];
	Value *index_ptr = params[1];

	Value *indexvalue = builder.CreateAlignedLoad(index_ptr,8);
	Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_ptr,indexvalue);
	Value *firstbyte_value = builder.CreateAlignedLoad(dataGEP,1);

	Value *encoding;
	{
		Value *shiftvalue = builder.CreateAShr(firstbyte_value,gen->getConstant(8,6));
		encoding = builder.CreateAnd(shiftvalue,gen->getConstant(8,0x03));
	}
	//print(gen,builder,"encoding: %d\n",encoding);

	BasicBlock *label_if_cond[encoding_count];
	BasicBlock *label_if_body[encoding_count];
	for(int i=0;i<encoding_count;i++){
		label_if_cond[i] = BasicBlock::Create(gen->context(),"if.cond."+i,fn);
		label_if_body[i] = BasicBlock::Create(gen->context(),"if.body."+i,fn);
	}
	BasicBlock *label_if_end = BasicBlock::Create(gen->context(),"if.end",fn);
	for(int i=0;i<encoding_count;i++){
		if(i==0){
			builder.CreateBr(label_if_cond[0]);
		}
		builder.SetInsertPoint(label_if_cond[i]);
		{
			Value *cmp_value = builder.CreateICmpEQ(encoding,gen->getConstant(8,funcEncoding[i]));
			if(i<encoding_count-1){
				builder.CreateCondBr(cmp_value,label_if_body[i],label_if_cond[i+1]);
			}else{
				builder.CreateCondBr(cmp_value,label_if_body[i],label_if_end);
			}
		}

		builder.SetInsertPoint(label_if_body[i]);
		{
			Value *ret_value = builder.CreateCall(nextFuncs[i],params);
			builder.CreateRet(ret_value);
		}
	}

	builder.SetInsertPoint(label_if_end);
	builder.CreateRet(gen->getConstant(64,0));

	gen->AddFunctionToJit(fn,(void**)&fn);
	return fn;


}


Function *genfunc_next(LlvmCodeGen *gen, ColumnInfo *info){

	switch(info->column.type){
	case TypeKind::BYTE:
		return genfunc_rle(gen,info);
	case TypeKind::SHORT:
		return genfunc_rle(gen,info);
	case TypeKind::INT:
		return genfunc_rle(gen,info);
	case TypeKind::LONG:
		return genfunc_rle(gen,info);
	case TypeKind::STRING:
		return genfunc_rle(gen,info);
	case TypeKind::VARCHAR:
		return genfunc_rle(gen,info);
	default:
		return NULL;
	}

}




//Function *genfunc_double(LlvmCodeGen *gen,ColumnInfo *info){
//
//	IRBuilder<> builder(gen->context());
//	LlvmCodeGen::FnPrototype proto(gen,"next_"+info->column.name,gen->int_type(64),false);
//	proto.AddArgument("data", gen->int_ptr_type(8));
//	proto.AddArgument("index", gen->int_ptr_type(64));
//	proto.AddArgument("offset",gen->int_type(64));
//	proto.AddArgument("runlength",gen->int_type(64));
//	proto.AddArgument("result",gen->int_ptr_type(8));
//
//	llvm::Value *params[5];
//	Function *fn = proto.GeneratePrototype(&builder,params);
//
//	Value *data_addr = params[0];
//	Value *index_addr = params[1];
//	Value *offset_value = params[2];
//	Value *length_value = params[3];
//	Value *result_ptr = builder.CreateBitCast(params[4],gen->double_ptr_type());
//
//	Value *end_value = builder.CreateAdd(offset_value,length_value);
//	Value *datadouble_ptr = builder.CreateBitCast(data_addr,gen->double_ptr_type());
//	Value *index_value = builder.CreateAlignedLoad(index_addr,8);
//	{
//		//index += length*8
//		Value *mulvalue = builder.CreateMul(length_value,gen->getConstant(64,8));
//		Value *addvalue = builder.CreateAdd(mulvalue,index_value);
//		builder.CreateAlignedStore(addvalue,index_addr,8);
//	}
//	Value *index64_value = builder.CreateUDiv(index_value,gen->getConstant(64,8));
//	Value *gap_value = builder.CreateSub(offset_value,index64_value);
//
//	Value *t_ptr = builder.CreateAlloca(gen->int_type(64));
//	builder.CreateAlignedStore(offset_value,t_ptr,8);
//
//	BasicBlock *label_for_cond = BasicBlock::Create(gen->context(),"for.cond",fn);
//	BasicBlock *label_for_body = BasicBlock::Create(gen->context(),"for.body",fn);
//	BasicBlock *label_for_inc = BasicBlock::Create(gen->context(),"for.inc",fn);
//	BasicBlock *label_for_end = BasicBlock::Create(gen->context(),"for.end",fn);
//
//	builder.CreateBr(label_for_cond);
//	builder.SetInsertPoint(label_for_cond);
//	{
//		Value *t_value = builder.CreateAlignedLoad(t_ptr,8);
//		Value *cmp_value = builder.CreateICmpULT(t_value,end_value);
//		builder.CreateCondBr(cmp_value,label_for_body,label_for_end);
//	}
//
//	builder.SetInsertPoint(label_for_body);
//	{
//		Value *t_value = builder.CreateAlignedLoad(t_ptr,8);
//		index64_value = builder.CreateSub(t_value,gap_value);
//		Value *dataGEP = builder.CreateGEP(builder.getDoubleTy(),datadouble_ptr,index64_value);
//		Value *double_value = builder.CreateAlignedLoad(dataGEP,8);
//
//		Value *resultGEP = builder.CreateGEP(builder.getDoubleTy(),result_ptr,t_value);
//		builder.CreateAlignedStore(double_value,resultGEP,8);
//		builder.CreateBr(label_for_inc);
//	}
//
//	builder.SetInsertPoint(label_for_inc);
//	{
//		Value *t_value = builder.CreateAlignedLoad(t_ptr,8);
//		Value *incvalue = builder.CreateAdd(t_value,gen->getConstant(64,1));
//		builder.CreateAlignedStore(incvalue,t_ptr,8);
//		builder.CreateBr(label_for_cond);
//	}
//
//	builder.SetInsertPoint(label_for_end);
//	builder.CreateRet(length_value);
//
//	gen->AddFunctionToJit(fn,(void **)&fn);
//	return fn;
//
//
//}
}

