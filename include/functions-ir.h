/*
 * functions-ir.h
 *
 *  Created on: Jul 11, 2016
 *      Author: teng
 */

#ifndef INCLUDE_FUNCTIONS_IR_H_
#define INCLUDE_FUNCTIONS_IR_H_

#include "../include/MemSpace.h"
#include "llvm-codegen.h"
#include "ColumnInfo.h"
#include "ProcessRow-ir.h"
#include <llvm/IR/TypeBuilder.h>

using namespace llvm;

namespace orc{

typedef  int64_t (*next_func)(unsigned char *, uint64_t &, uint64_t, uint64_t, void *);


static Value *getGlobalArray(LlvmCodeGen *gen, IRBuilder<> &builder, std::string gvar, Value *index){
	std::vector<Value *> arrayindex;
	arrayindex.push_back(gen->getConstant(64,0));
	arrayindex.push_back(index);
	GlobalVariable *var = gen->module()->getGlobalVariable(gvar);
	Value *cfb_GEP = builder.CreateGEP(var,arrayindex);
	Value *cfb_value32 = builder.CreateAlignedLoad(cfb_GEP,8);
	return builder.CreateZExt(cfb_value32,gen->int_type(64));
}


static Function *printf_prototype(LlvmCodeGen *gen) {

  FunctionType *printf_type = TypeBuilder<int(char *, ...), false>::get(gen->context());

  Function *func = cast<Function>(gen->module()->getOrInsertFunction("printf", printf_type,
		  AttributeSet().addAttribute(gen->context(), 1U, Attribute::NoAlias)));
  return func;
}

static Constant* geti8StrVal(LlvmCodeGen *gen, char const* str) {

	  Constant* strConstant = ConstantDataArray::getString(gen->context(), str);
	  GlobalVariable* GVStr =
		  new GlobalVariable(*gen->module(), strConstant->getType(), true,
							 GlobalValue::PrivateLinkage, strConstant,".str");
	  GVStr->setAlignment(1);
	  Constant* zero = Constant::getNullValue(gen->int_type(32));
	  Constant* indices[] = {zero, zero};
	  Constant* strVal = ConstantExpr::getGetElementPtr(strConstant->getType(),GVStr,indices);
	  return strVal;
}

static void print(LlvmCodeGen *gen, IRBuilder<> &builder, char const *str, Value *value=NULL) {

  std::vector<Value*> params;
  params.push_back(geti8StrVal(gen,str));
  if(value!=NULL)
	  params.push_back(value);
  builder.CreateCall(printf_prototype(gen), params);

}

void advance(LlvmCodeGen *gen, IRBuilder<> &builder, Value *data_ptr, Value *index_ptr, Value *bytes[], int byte_num);
Function *genfunc_unZigZag(LlvmCodeGen *gen, int result_bitwidth);

Function *genfunc_readLongs(LlvmCodeGen *gen, EncodingInfo *info, int result_bitwidth, bool issigned, bool optimize);
Function *genfunc_readLongBE(LlvmCodeGen *gen, EncodingInfo *info, int result_bitwidth);
Function *genfunc_readVLong(LlvmCodeGen *gen, int result_bitwidth, bool issigned);

Function *genfunc_next(LlvmCodeGen *gen, ColumnInfo *info);


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

inline Function *genfunc_process(LlvmCodeGen *gen, Type * type){

	PointerType *strtype = orc::MemSpace::getMemSpacePtr(gen);
	std::vector<LlvmCodeGen::NamedVariable> vars;
	vars.push_back(LlvmCodeGen::NamedVariable("value",type));
	vars.push_back(LlvmCodeGen::NamedVariable("offset",gen->int_type(64)));
	vars.push_back(LlvmCodeGen::NamedVariable("space",strtype));

	Function *process_func = genfunc_slot(gen,vars,gen->void_type(),"process",true);
	return process_func;
};

inline void processValue(LlvmCodeGen *gen, IRBuilder<> &builder, Value *value, Value *offset, Value *space_ptr){

	std::vector<Value *> call_params;

	call_params.push_back(value);
	call_params.push_back(offset);
	call_params.push_back(space_ptr);

	builder.CreateCall(genfunc_process(gen,value->getType()),call_params);
};


Function *genfunc_process(LlvmCodeGen *gen, std::vector<ColumnOP *> ops);
//Function *genfunc_hash(LlvmCodeGen *gen);


}


#endif /* INCLUDE_FUNCTIONS_IR_H_ */
