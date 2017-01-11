/*
 * ProcessRow-ir.cpp
 *
 *  Created on: Aug 9, 2016
 *      Author: teng
 */
#include <iostream>
#include "llvm-codegen.h"
#include "ProcessRow-ir.h"
#include "../include/MemSpace.h"

using namespace llvm;
namespace orc{

Value *Statement::generate(IRBuilder<> &builder, Value *rowvalue,Value *offset,Value *result_ptr){
		if(value){
			return value;
		}else{
			switch(code){
			case RETURNVALUE:
				return rowvalue;
			case PRICETDISCOUNT:
			{
				Value *result_GEP = builder.CreateGEP(PointerType::get(builder.getDoubleTy(),0),result_ptr,builder.getInt64(0));
				result_ptr = builder.CreateAlignedLoad(result_GEP,8);
				result_GEP = builder.CreateGEP(builder.getDoubleTy(),result_ptr,offset);
				Value *price_value = builder.CreateLoad(result_GEP);
				Value *minus_value = builder.CreateFSub(ConstantFP::get(builder.getDoubleTy(),1),rowvalue);
				value = builder.CreateFMul(price_value,minus_value);
				return value;
			}
			case PRICETDISCOUNTTTAX:
			{
				Value *result_GEP = builder.CreateGEP(PointerType::get(builder.getDoubleTy(),0),result_ptr,builder.getInt64(0));
				result_ptr = builder.CreateAlignedLoad(result_GEP,8);
				result_GEP = builder.CreateGEP(builder.getDoubleTy(),result_ptr,offset);
				Value *priceTimesdiscount_value = builder.CreateLoad(result_GEP);
				Value *plus_value = builder.CreateFAdd(ConstantFP::get(builder.getDoubleTy(),1),rowvalue);
				value = builder.CreateFMul(priceTimesdiscount_value,plus_value);
				return value;
			}
			default:
				break;
			}
			return NULL;
		}
	}


Function *genfunc_process(LlvmCodeGen *gen, std::vector<ColumnOP *> ops){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"process",builder.getVoidTy(),true);
	Type *double_ptr = PointerType::get(PointerType::get(builder.getDoubleTy(),0),0);
	proto.AddArgument("rowvalue", gen->double_type());
	proto.AddArgument("offset",builder.getInt64Ty());
	proto.AddArgument("space", orc::MemSpace::getMemSpacePtr(gen));

	llvm::Value *params[3];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *rowvalue_value = params[0];
	Value *offset_value = params[1];
	Value *space_ptr = params[2];

	if(true)
	{
		Value *result_ptr = NULL;
		Value *aggresult_ptr = NULL;
		for(int i=0;i<ops.size();i++){
			ColumnOP *op = ops[i];
			switch(op->op){
			case store:
			{
				if(!result_ptr){
					result_ptr = orc::MemSpace::getField(gen,builder,space_ptr,MemSpaceField::doubleResult);
				}
				Value *processed_value = op->statement->generate(builder,rowvalue_value,offset_value,result_ptr);
				//save result
				Value *result_GEP = builder.CreateGEP(gen->double_ptr_type(),result_ptr,builder.getInt64(op->dest));
				result_ptr = builder.CreateAlignedLoad(result_GEP,8);
				result_GEP = builder.CreateGEP(gen->double_type(),result_ptr,offset_value);
				builder.CreateStore(processed_value,result_GEP);
				break;
			}
			case agg:
			{
				if(!result_ptr){
					result_ptr = orc::MemSpace::getField(gen,builder,space_ptr,MemSpaceField::doubleResult);
				}
				if(!aggresult_ptr){

					Value *hashresult_ptr = orc::MemSpace::getField(gen,builder,space_ptr,MemSpaceField::hashResult);
					Value *aggpos_GEP = builder.CreateGEP(builder.getInt64Ty(),hashresult_ptr,offset_value);
					Value *aggpos_value = builder.CreateAlignedLoad(aggpos_GEP,8);
					aggresult_ptr = MemSpace::getField(gen,builder,space_ptr,MemSpaceField::doubleAggResult);
					Value *aggresult_GEP = builder.CreateGEP(gen->double_ptr_type(),aggresult_ptr,aggpos_value);
					aggresult_ptr = builder.CreateAlignedLoad(aggresult_GEP,8);

				}

				Value *aggresult_GEP = builder.CreateGEP(gen->double_type(),aggresult_ptr,builder.getInt64(op->dest));
				Value *aggresult_value = builder.CreateLoad(aggresult_GEP);
				Value *processd_value = op->statement->generate(builder,rowvalue_value,offset_value,result_ptr);
				Value *add_value = builder.CreateFAdd(aggresult_value,processd_value);
				builder.CreateStore(add_value,aggresult_GEP);
				break;
			}
			default:
				break;

			}
		}

		//aggregation

	}
	builder.CreateRetVoid();

	return fn;
}


}

