/*
 * TempSpace.h
 *
 *  Created on: Aug 6, 2016
 *      Author: teng
 */

#ifndef TEMPSPACE_H_
#define TEMPSPACE_H_

#include "llvm-codegen.h"

using namespace llvm;
namespace tengdb{

enum MemSpaceField{
	bitMap = 0,
	hashResult = 1,
	intResult = 2,
	doubleResult = 3,
	intAggResult = 4,
	doubleAggResult = 5
};

struct MemSpace{

	char *bitMap;
	uint64_t *hashResult;
	int64_t **intResult;
	double **doubleResult;
	int64_t **intAggResult;
	double **doubleAggResult;

	static inline llvm::StructType *getMemSpaceType(LlvmCodeGen *gen){

		Module* mod = gen->module();
		StructType *type = mod->getTypeByName("struct.MemSpace");
		if(!type)
		{
			type = StructType::create(gen->context(),"struct.MemSpace");
			std::vector<Type*> fields;

			fields.push_back(gen->int_ptr_type(8));//bitmap
			fields.push_back(gen->int_ptr_type(64));//hashResult
			fields.push_back(gen->int_ptr_ptr_type(64));//intResult
			fields.push_back(gen->double_ptr_ptr_type());//doubleResult
			fields.push_back(gen->int_ptr_ptr_type(64));//intAggResult
			fields.push_back(gen->double_ptr_ptr_type());//doubleAggResult

			type->setBody(fields,false);
		}

		return type;
	}

	static inline llvm::PointerType *getMemSpacePtr(LlvmCodeGen *gen){

		return PointerType::get(getMemSpaceType(gen),0);

	}
	static inline Value *getField(LlvmCodeGen *gen, IRBuilder<> &builder, Value *space_ptr, MemSpaceField field){

		std::vector<Value *> array;
		array.push_back(builder.getInt32(0));
		array.push_back(builder.getInt32(field));
		Value *GEP= builder.CreateGEP(getMemSpaceType(gen),space_ptr,array);
		return builder.CreateAlignedLoad(GEP,8);

	}

};




}



#endif /* TEMPSPACE_H_ */
