/*
 * functions-ir.h
 *
 *  Created on: Jul 11, 2016
 *      Author: teng
 */

#ifndef INCLUDE_FUNCTIONS_IR_H_
#define INCLUDE_FUNCTIONS_IR_H_

#include "llvm-codegen.h"
#include "ColumnInfo.h"

using namespace llvm;

namespace orc{

Function *genfunc_unZigZag(LlvmCodeGen *gen, EncodingInfo info);
Function *genfunc_decodeBitWidth(LlvmCodeGen *gen, EncodingInfo info);
Function *genfunc_readLongs(LlvmCodeGen *gen, EncodingInfo info);
Function *genfunc_readLongBE(LlvmCodeGen *gen, EncodingInfo info);
Function *genfunc_readVsLong(LlvmCodeGen *gen, EncodingInfo info);
Function *genfunc_readVuLong(LlvmCodeGen *gen, EncodingInfo info);
Function *genfunc_nextDirect(LlvmCodeGen *gen, EncodingInfo info);
Function *genfunc_nextRepeat(LlvmCodeGen *gen, EncodingInfo info);
Function *genfunc_nextDelta(LlvmCodeGen *gen, EncodingInfo info);
Function *genfunc_nextPatched(LlvmCodeGen *gen, EncodingInfo info);

Function *genfunc_readDouble(LlvmCodeGen *gen, ColumnInfo info);
Function *genfunc_next(LlvmCodeGen *gen, ColumnInfo info);

inline Function *genfunc_slot(LlvmCodeGen *gen,std::vector<LlvmCodeGen::NamedVariable> variables, Type *rettype, std::string funcname, bool isinline){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,funcname,rettype,isinline);
	for(int i=0;i<variables.size();i++){
		proto.AddArgument(variables[i]);
	}
	llvm::Value *params[variables.size()];
	Function *fn = proto.GeneratePrototype(&builder,params);
	if(!rettype->isVoidTy()){
		Value *retptr = builder.CreateAlloca(rettype);
		Value *retvalue = builder.CreateLoad(rettype,retptr);
		builder.CreateRet(retvalue);
	}else{
		builder.CreateRetVoid();
	}
	return fn;
};

//Function *genfunc_process(LlvmCodeGen *gen);
//Function *genfunc_hash(LlvmCodeGen *gen);


}


#endif /* INCLUDE_FUNCTIONS_IR_H_ */
