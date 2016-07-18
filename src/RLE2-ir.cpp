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
#include "llvm-codegen.h"
#include "ColumnInfo.h"
#include "functions-ir.h"

using namespace llvm;

namespace orc{

Function *genfunc_nextDirect(LlvmCodeGen *gen, EncodingInfo info){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"nextDirect",IntegerType::getInt64Ty(gen->context()),false);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("result", gen->int_ptr_type(64));
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));

	llvm::Value *params[4];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *data_addr = builder.CreateAlloca(gen->int_ptr_type(8));
	builder.CreateAlignedStore(params[0],data_addr,8);
	Value *result_addr = builder.CreateAlloca(gen->int_ptr_type(64));
	builder.CreateAlignedStore(params[1],result_addr,8);
	Value *index_addr = builder.CreateAlloca(gen->int_ptr_type(64));
	builder.CreateAlignedStore(params[2],index_addr,8);
	Value *offset = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(params[3],offset,8);


	Value *indexaddr = builder.CreateAlignedLoad(index_addr,8);
	Value *dataaddr = builder.CreateAlignedLoad(data_addr,8);
	Value *resultaddr = builder.CreateAlignedLoad(result_addr,8);

	//firstbyte = data[index++];
	Value *indexvalue = builder.CreateAlignedLoad(indexaddr,8);
	Value *incvalue = builder.CreateAdd(indexvalue,gen->getConstant(64,1));
	builder.CreateAlignedStore(incvalue,indexaddr,8);
	Value *dataGEP = builder.CreateGEP(gen->int_type(8),dataaddr,indexvalue);
	Value *firstbytevalue = builder.CreateAlignedLoad(dataGEP,1);

	//secondbyte = data[index++];
	Value *indexvalue1 = builder.CreateAlignedLoad(indexaddr,8);
	Value *incvalue1 = builder.CreateAdd(indexvalue1,gen->getConstant(64,1));
	builder.CreateAlignedStore(incvalue1,indexaddr,8);
	Value *dataGEP1 = builder.CreateGEP(gen->int_type(8),dataaddr,indexvalue1);
	Value *secondbytevalue = builder.CreateAlignedLoad(dataGEP1,1);

	Value *shiftvalue1 = builder.CreateAShr(firstbytevalue,gen->getConstant(8,1));
	Value *andvalue1 = builder.CreateAnd(shiftvalue1,gen->getConstant(8,0x1f));
	Value *andvalue_32 = builder.CreateSExt(andvalue1,gen->int_type(32));
	Value *decodedvalue = builder.CreateCall(genfunc_decodeBitWidth(gen,info),andvalue_32);
	Value *bitWidth_value = builder.CreateSExt(decodedvalue,gen->int_type(64));

	//runLength = (((firstbyte&0x01)<<8) | secondbyte) + 1
	Value *andvalue = builder.CreateAnd(firstbytevalue,gen->getConstant(8,1));
	Value *andvalue_64 = builder.CreateSExt(andvalue,gen->int_type(64));
	Value *shiftvalue = builder.CreateShl(andvalue_64,gen->getConstant(64,8));
	Value *secondbytevalue_64 = builder.CreateZExt(secondbytevalue,gen->int_type(64));
	Value *orvalue = builder.CreateOr(shiftvalue, secondbytevalue_64);
	Value *incvalue2 = builder.CreateAdd(orvalue,gen->getConstant(64,1));

	std::vector<Value *> call_params;
	Value *offsetvalue = builder.CreateAlignedLoad(offset,8);
	call_params.push_back(dataaddr);
	call_params.push_back(resultaddr);
	call_params.push_back(indexaddr);
	call_params.push_back(offsetvalue);
	call_params.push_back(incvalue2);
	call_params.push_back(bitWidth_value);

	Value *ret = builder.CreateCall(genfunc_readLongs(gen, info),call_params);

	builder.CreateRet(ret);
	gen->AddFunctionToJit(fn,(void **)&fn);
	return fn;

}


Function *genfunc_nextRepeat(LlvmCodeGen *gen, EncodingInfo info){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"nextRepeat",IntegerType::getInt64Ty(gen->context()),false);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("result", gen->int_ptr_type(64));
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));

	llvm::Value *params[4];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *data_addr = builder.CreateAlloca(gen->int_ptr_type(8));
	builder.CreateAlignedStore(params[0],data_addr,8);
	Value *result_addr = builder.CreateAlloca(gen->int_ptr_type(64));
	builder.CreateAlignedStore(params[1],result_addr,8);
	Value *index_addr = builder.CreateAlloca(gen->int_ptr_type(64));
	builder.CreateAlignedStore(params[2],index_addr,8);
	Value *offset_ptr = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(params[3],offset_ptr,8);


	Value *indexaddr = builder.CreateAlignedLoad(index_addr,8);
	Value *dataaddr = builder.CreateAlignedLoad(data_addr,8);
	Value *resultaddr = builder.CreateAlignedLoad(result_addr,8);

	//firstByte = data[index++];
	Value *indexvalue = builder.CreateAlignedLoad(indexaddr,8);
	Value *incvalue = builder.CreateAdd(indexvalue,gen->getConstant(64,1));
	builder.CreateAlignedStore(incvalue,indexaddr,8);
	Value *dataGEP = builder.CreateGEP(gen->int_type(8),dataaddr,indexvalue);
	Value *firstbyte_value = builder.CreateAlignedLoad(dataGEP,1);

	Value *value_ptr = builder.CreateAlloca(gen->int_type(64));
	Value *value_value;
	{
		//byteSize = (firstByte >> 3) & 0x07 + 1;
		//value = readLongBE(byteSize);
		Value *shiftvalue = builder.CreateAShr(firstbyte_value,gen->getConstant(8,3));
		Value *andvalue = builder.CreateAnd(shiftvalue,gen->getConstant(8,0x07));
		Value *andvalue_64 = builder.CreateSExt(andvalue,gen->int_type(64));
		Value *bytesize = builder.CreateAdd(andvalue_64,gen->getConstant(64,1));
		std::vector<Value *> call_params;
		call_params.push_back(dataaddr);
		call_params.push_back(indexaddr);
		call_params.push_back(bytesize);
		value_value = builder.CreateCall(genfunc_readLongBE(gen,info),call_params);
		//value = unZigZag(value); for signed
		if(info.issigned){
			std::vector<Value *> call_params;
			call_params.push_back(value_value);
			Value *unzigzagedvalue_value = builder.CreateCall(genfunc_unZigZag(gen,info),call_params);
			builder.CreateAlignedStore(unzigzagedvalue_value,value_ptr,64);
		}else{
			builder.CreateAlignedStore(value_value,value_ptr,64);
		}
	}
	value_value = builder.CreateAlignedLoad(value_ptr,64);


	Value *runLength_value;
	{
		//runLength = firstByte & 0x07 + 3;
		Value *andvalue = builder.CreateAnd(firstbyte_value,gen->getConstant(8,0x07));
		Value *andvalue_64 = builder.CreateSExt(andvalue,gen->int_type(64));
		runLength_value = builder.CreateAdd(andvalue_64,gen->getConstant(64,3));//MIN_REPEAT
	}

	Value *end_value;
	Value *pos_ptr = builder.CreateAlloca(gen->int_type(64));
	{
		//pos = offset;
		//end = runLength + offset;
		Value *offset_value = builder.CreateAlignedLoad(offset_ptr,8);
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
		Value *resultaddr = builder.CreateAlignedLoad(result_addr,64);
		Value *resultGEP = builder.CreateGEP(gen->int_type(64),resultaddr,pos_value);
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
	//return runLength;
	builder.CreateRet(runLength_value);

	return fn;
}


Function *genfunc_nextDelta(LlvmCodeGen *gen,EncodingInfo info){

	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"nextDelta",IntegerType::getInt64Ty(gen->context()),false);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("result", gen->int_ptr_type(64));
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));

	llvm::Value *params[4];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *data_addr = params[0];
	Value *result_addr = params[1];
	Value *index_addr = params[2];
	Value *offset_value = params[3];

	Value *firstbyte_value;
	Value *secondbyte_value;
	//firstbyte = data[index++];
	{
		Value *indexvalue = builder.CreateAlignedLoad(index_addr,8);
		Value *incvalue = builder.CreateAdd(indexvalue,gen->getConstant(64,1));
		builder.CreateAlignedStore(incvalue,index_addr,8);
		Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_addr,indexvalue);
		firstbyte_value = builder.CreateAlignedLoad(dataGEP,1);
	}
	//secondbyte = data[index++];
	{
		Value *indexvalue = builder.CreateAlignedLoad(index_addr,8);
		Value *incvalue = builder.CreateAdd(indexvalue,gen->getConstant(64,1));
		builder.CreateAlignedStore(incvalue,index_addr,8);
		Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_addr,indexvalue);
		secondbyte_value = builder.CreateAlignedLoad(dataGEP,1);
	}
	Value *fbo_value;
	//fbo = (firstbyte >> 1)&0x1f
	{
		Value *shiftvalue = builder.CreateAShr(firstbyte_value,gen->getConstant(8,1));
		Value *andvalue = builder.CreateAnd(shiftvalue,gen->getConstant(8,0x1f));
		fbo_value = builder.CreateSExt(andvalue,gen->int_type(32));
	}

	BasicBlock *label_if_cond_fbo = BasicBlock::Create(gen->context(),"if.cond.fbo",fn);
	BasicBlock *label_if_then_fbo = BasicBlock::Create(gen->context(),"if.then.fbo",fn);
	BasicBlock *label_if_end_fbo = BasicBlock::Create(gen->context(),"if.end.fbo",fn);

	Value *bitSize_ptr = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(gen->getConstant(64,0),bitSize_ptr,8);

	builder.CreateBr(label_if_cond_fbo);
	builder.SetInsertPoint(label_if_cond_fbo);
	//if(fbo != 0)
	{
		Value *cmp_value = builder.CreateICmpNE(fbo_value,gen->getConstant(32,0));
		builder.CreateCondBr(cmp_value,label_if_then_fbo,label_if_end_fbo);
	}


	//bitSize = decodeBitWidth(fbo);
	builder.SetInsertPoint(label_if_then_fbo);
	{
		Value *bitSize_value_32 = builder.CreateCall(genfunc_decodeBitWidth(gen,info),fbo_value);
		Value *bitSize_value = builder.CreateSExt(bitSize_value_32,gen->int_type(64));
		builder.CreateAlignedStore(bitSize_value,bitSize_ptr,8);
		builder.CreateBr(label_if_end_fbo);
	}

	builder.SetInsertPoint(label_if_end_fbo);
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

	//firstvalue = readVsLong(data,index);
	Function *readfunc;
	if(info.issigned){
		readfunc = genfunc_readVsLong(gen,info);
	}else{
		readfunc = genfunc_readVuLong(gen,info);
	}
	std::vector<Value *> call_params;
	call_params.push_back(data_addr);
	call_params.push_back(index_addr);
	Value *firstValue_value = builder.CreateCall(readfunc,call_params);
	//deltabase = readVsLong(data,index);
	Value *deltaBase_value = builder.CreateCall(genfunc_readVsLong(gen,info),call_params);
	//pos = offset;
	//end = offset + runLength;
	//prevValue = firstValue;
	Value *pos_ptr = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(offset_value,pos_ptr,8);
	Value *end_value = builder.CreateAdd(offset_value,runLength_value);
	Value *prevValue_ptr = builder.CreateAlloca(gen->int_type(64));
	builder.CreateAlignedStore(firstValue_value,prevValue_ptr,8);

	//result[pos++] = firstValue;
	{
		Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
		Value *incvalue = builder.CreateAdd(pos_value,gen->getConstant(64,1));
		builder.CreateAlignedStore(incvalue,pos_ptr,8);
		Value *resultGEP = builder.CreateGEP(gen->int_type(64),result_addr,pos_value);
		builder.CreateAlignedStore(firstValue_value,resultGEP,8);
	}

	BasicBlock *label_if_cond_bitsize = BasicBlock::Create(gen->context(),"if.cond.bitsize",fn);
	BasicBlock *label_if_then_bitsize = BasicBlock::Create(gen->context(),"if.then.bitsize",fn);
	BasicBlock *label_if_else_bitsize = BasicBlock::Create(gen->context(),"if.else.bitsize",fn);
	BasicBlock *label_if_end_bitsize = BasicBlock::Create(gen->context(),"if.end.bitsize",fn);

	builder.CreateBr(label_if_cond_bitsize);
	builder.SetInsertPoint(label_if_cond_bitsize);
	//if(bitSize == 0)
	{
		Value *bitSize_value = builder.CreateAlignedLoad(bitSize_ptr,8);
		Value *cmp_value = builder.CreateICmpEQ(bitSize_value,gen->getConstant(64,0));
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
			Value *resultGEP = builder.CreateGEP(gen->int_type(64),result_addr,pos_value);
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
	}


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
			Value *resultGEP = builder.CreateGEP(gen->int_type(64),result_addr,pos_value);
			builder.CreateAlignedStore(addvalue,resultGEP,8);
			builder.CreateAlignedStore(addvalue,prevValue_ptr,8);
			builder.CreateBr(label_if_end_else);
		}
		builder.SetInsertPoint(label_if_end_else);
		//remaining = end - pos;
		//readLongs(data, result, index, pos, remaining, bitsize);
		Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
		Value *remaining_value = builder.CreateSub(end_value,pos_value);
		Value *bitSize_value = builder.CreateAlignedLoad(bitSize_ptr,8);
		std::vector<Value *> call_params;
		call_params.push_back(data_addr);
		call_params.push_back(result_addr);
		call_params.push_back(index_addr);
		call_params.push_back(pos_value);
		call_params.push_back(remaining_value);
		call_params.push_back(bitSize_value);
		Function *func_readLongs = genfunc_readLongs(gen,info);
		builder.CreateCall(func_readLongs,call_params);

		BasicBlock *label_if_cond_deltabase = BasicBlock::Create(gen->context(),"if.cond.deltabase",fn);
		BasicBlock *label_if_then_deltabase = BasicBlock::Create(gen->context(),"if.then.deltabase",fn);
		BasicBlock *label_if_else_deltabase = BasicBlock::Create(gen->context(),"if.else.deltabase",fn);
		builder.CreateBr(label_if_cond_deltabase);
		//if(deltabase < 0)
		builder.SetInsertPoint(label_if_cond_deltabase);
		{
			Value *cmp_value = builder.CreateICmpULT(deltaBase_value,gen->getConstant(64,0));
			builder.CreateCondBr(cmp_value,label_if_then_deltabase,label_if_else_deltabase);
		}
		builder.SetInsertPoint(label_if_then_deltabase);
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
			{
				Value *prevValue_value = builder.CreateAlignedLoad(prevValue_ptr,8);
				Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
				Value *resultGEP = builder.CreateGEP(gen->int_type(64),result_addr,pos_value);
				Value *result_value = builder.CreateAlignedLoad(resultGEP,8);
				Value *subvalue = builder.CreateSub(prevValue_value,result_value);
				builder.CreateAlignedStore(subvalue,resultGEP,8);
				builder.CreateAlignedStore(subvalue,prevValue_ptr,8);
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
		builder.SetInsertPoint(label_if_else_deltabase);
		{
			BasicBlock *label_for_cond_else = BasicBlock::Create(gen->context(),"for.cond.else",fn);
			BasicBlock *label_for_body_else = BasicBlock::Create(gen->context(),"for.body.else",fn);
			BasicBlock *label_for_inc_else = BasicBlock::Create(gen->context(),"for.inc.else",fn);
			builder.CreateBr(label_for_cond_else);
			builder.SetInsertPoint(label_for_cond_else);
			//for(;pos<end;pos++);
			{
				Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
				Value *cmp_value = builder.CreateICmpULT(pos_value,end_value);
				builder.CreateCondBr(cmp_value,label_for_body_else,label_if_end_bitsize);
			}
			//prevValue = result[pos] = prevValue + result[pos];
			builder.SetInsertPoint(label_for_body_else);
			{
				Value *prevValue_value = builder.CreateAlignedLoad(prevValue_ptr,8);
				Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
				Value *resultGEP = builder.CreateGEP(gen->int_type(64),result_addr,pos_value);
				Value *result_value = builder.CreateAlignedLoad(resultGEP,8);
				Value *addvalue = builder.CreateAdd(prevValue_value,result_value);
				builder.CreateAlignedStore(addvalue,resultGEP,8);
				builder.CreateAlignedStore(addvalue,prevValue_ptr,8);
				builder.CreateBr(label_for_inc_else);
			}
			//pos++;
			builder.SetInsertPoint(label_for_inc_else);
			{
				Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
				Value *incvalue = builder.CreateAdd(pos_value,gen->getConstant(64,1));
				builder.CreateAlignedStore(incvalue,pos_ptr,8);
				builder.CreateBr(label_for_cond_else);
			}
		}
	}

	builder.SetInsertPoint(label_if_end_bitsize);
	builder.CreateRet(runLength_value);

	//fn->dump();
	return fn;

}


Function *genfunc_nextPatched(LlvmCodeGen *gen, EncodingInfo info){

	return NULL;

}

Function *genfunc_next(LlvmCodeGen *gen, ColumnInfo info){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"nextFunc",IntegerType::getInt64Ty(gen->context()),false);
	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("result", gen->int_ptr_type(64));
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));

	llvm::Value *params[4];
	Function *fn = proto.GeneratePrototype(&builder,&params[0]);

	int encoding_count = info.hasdelta + info.hasdirect + info.hasrepeat + info.haspatched;
	assert(encoding_count>0);
	if(encoding_count==1){
		Function *nextfunc;
		if(info.hasrepeat){
			nextfunc = genfunc_nextRepeat(gen, info.repeatInfo);
		}else if(info.hasdirect){
			nextfunc = genfunc_nextDirect(gen, info.directInfo);
		}else if(info.hasdelta){
			nextfunc = genfunc_nextDelta(gen,info.deltaInfo);
		}else if(info.haspatched){
			nextfunc = genfunc_nextPatched(gen,info.patchedInfo);
		}
		Value *ret = builder.CreateCall(nextfunc,params);
		builder.CreateRet(ret);
		gen->AddFunctionToJit(fn,(void**)&fn);
		return fn;
	}

	Function *nextFuncs[encoding_count];
	int funcEncoding[encoding_count];
	int count = 0;
	if(info.hasrepeat){
		funcEncoding[count] = 0;
		nextFuncs[count] = genfunc_nextRepeat(gen, info.repeatInfo);
		count++;
	}
	if(info.hasdirect){
		funcEncoding[count] = 1;
		nextFuncs[count] = genfunc_nextDirect(gen, info.directInfo);
		count++;
	}
	if(info.haspatched){
		funcEncoding[count] = 2;
		nextFuncs[count] = genfunc_nextPatched(gen, info.patchedInfo);
		count++;
	}
	if(info.hasdelta){
		funcEncoding[count] = 3;
		nextFuncs[count] = genfunc_nextDelta(gen, info.deltaInfo);
	}


	Value *data_addr = params[0];
	Value *result_addr = params[1];
	Value *index_addr = params[2];
	Value *offset_value = params[3];

	Value *indexvalue = builder.CreateAlignedLoad(index_addr,8);
	Value *dataGEP = builder.CreateGEP(gen->int_type(8),data_addr,indexvalue);
	Value *firstbyte_value = builder.CreateAlignedLoad(dataGEP,1);

	Value *encoding;
	{
		Value *shiftvalue = builder.CreateAShr(firstbyte_value,gen->getConstant(8,6));
		encoding = builder.CreateAnd(shiftvalue,gen->getConstant(8,0x03));
	}

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



}

