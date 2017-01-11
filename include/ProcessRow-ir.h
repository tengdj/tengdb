/*
 * ProcessRow-ir.h
 *
 *  Created on: Aug 10, 2016
 *      Author: teng
 */

#ifndef SRC_PROCESSROW_IR_H_
#define SRC_PROCESSROW_IR_H_

#include "llvm-codegen.h"
using namespace llvm;
namespace orc{

enum HardCode{
	RETURNVALUE,
	PRICETDISCOUNT,
	PRICETDISCOUNTTTAX

};

class Statement{
	Value *value;
	HardCode code;
public:
	Statement(HardCode code){
		this->code = code;
		value = NULL;
	}
	Value *generate(IRBuilder<> &builder, Value *rowvalue,Value *offset, Value *result_ptr);

};

enum OP{
	store,
	agg
};

struct ColumnOP{

	OP op;
	Statement *statement;
	uint64_t dest;
};

}



#endif /* SRC_PROCESSROW_IR_H_ */
