/*
 * vectorization-ir.h
 *
 *  Created on: Jan 5, 2017
 *      Author: teng
 */

#ifndef INCLUDE_VECTORIZATION_IR_H_
#define INCLUDE_VECTORIZATION_IR_H_
#include "llvm-codegen.h"

using namespace llvm;
namespace orc{
void initializeVectorization(LlvmCodeGen *gen);

Value *unZigZag32(LlvmCodeGen *gen, IRBuilder<> &builder, Value *data, Value *zero, Value *one);
void vec_unZigZag16(LlvmCodeGen *gen, IRBuilder<> &builder, Value *data);

Value *_mm_shuffle_epi8(LlvmCodeGen *gen, IRBuilder<> &builder, Value *byte_value, Value *mask_value);
Value *_mm256_shuffle_epi8(LlvmCodeGen *gen, IRBuilder<> &builder, Value *byte_value, Value *mask_value);
Value *_mm256_permutevar8x32_epi32(LlvmCodeGen *gen, IRBuilder<> &builder, Value *byte_value, Value *mask_value);

Value *_mm_set_epi8(LlvmCodeGen *gen,IRBuilder<> &builder, int value[]);
Value *_mm256_set_epi8(LlvmCodeGen *gen,IRBuilder<> &builder, int value[]);
Value *_mm256_set_epi32(LlvmCodeGen *gen,IRBuilder<> &builder, int value[]);


}

#endif /* INCLUDE_VECTORIZATION_IR_H_ */
