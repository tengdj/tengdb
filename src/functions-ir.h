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

Function *genfunc_next(LlvmCodeGen *gen, ColumnInfo info);
}


#endif /* INCLUDE_FUNCTIONS_IR_H_ */
