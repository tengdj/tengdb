/*
 * Testllvm.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: teng
 */


#include "llvm-codegen.h"
#include <bitset>
#include <iostream>
#include "Table.h"
#include "Scanner.h"
#include "functions-ir.h"
#include "vectorization-ir.h"

using namespace llvm;
using namespace std;
using namespace orc;




Function *getfunc128(LlvmCodeGen *gen){
	 // Type Definitions
	 VectorType* type_8_16 = VectorType::get(IntegerType::get(gen->context(), 8), 16);
	 PointerType* type_8_16_pointer = PointerType::get(type_8_16, 0);

	 // Function Declarations

	 IRBuilder<> builder(gen->context());
	 LlvmCodeGen::FnPrototype proto(gen,"test",builder.getVoidTy(),false);
	 proto.AddArgument("result",gen->int_ptr_type(32));
	 proto.AddArgument("bytes",gen->int_ptr_type(8));
	 Value *params[2];
	 Function *func = proto.GeneratePrototype(&builder,&params[0]);
	 Value* result_ptr = params[0];
	 Value* byte_ptr = params[1];

	 int mask[]{2,1,0,-1,5,4,3,-1,8,7,6,-1,11,10,9,-1};
	 Value *mask_value = _mm_set_epi8(gen,builder,mask);
	 {

	  Value *ptr_byte_8_16 = builder.CreateBitCast(byte_ptr, type_8_16_pointer);
	  Value *byteGEP = builder.CreateGEP(type_8_16,ptr_byte_8_16,builder.getInt32(0));
	  Value *byte_value = builder.CreateAlignedLoad(byteGEP,1);

	  Value *shuffled_value = _mm_shuffle_epi8(gen,builder,byte_value,mask_value);

	  Value *resultGEP = builder.CreateGEP(gen->int_type(32),result_ptr,builder.getInt64(0));
	  resultGEP = builder.CreateBitCast(resultGEP,type_8_16_pointer);

	  builder.CreateAlignedStore(shuffled_value,resultGEP,16);

	  builder.CreateRetVoid();
	 }

	 return func;
}

Function *getfunc256(LlvmCodeGen *gen){
	 // Type Definitions
	 VectorType* type_8_32 = VectorType::get(IntegerType::get(gen->context(), 8), 32);
	 PointerType* type_8_32_pointer = PointerType::get(type_8_32, 0);
	 VectorType* type_32_8 = VectorType::get(IntegerType::get(gen->context(), 32), 8);
	 PointerType* type_32_8_pointer = PointerType::get(type_32_8, 0);

	 // Function Declarations

	 IRBuilder<> builder(gen->context());
	 LlvmCodeGen::FnPrototype proto(gen,"test",builder.getVoidTy(),false);
	 proto.AddArgument("result",gen->int_ptr_type(32));
	 proto.AddArgument("bytes",gen->int_ptr_type(8));
	 Value *params[2];
	 Function *func = proto.GeneratePrototype(&builder,&params[0]);
	 Value* result_ptr = params[0];
	 Value* byte_ptr = params[1];

	 int permute_mask[]{0,1,2,3,3,4,5,6};
	 int shuffle_mask[]{2,1,0,-1,5,4,3,-1,8,7,6,-1,11,10,9,-1,2,1,0,-1,5,4,3,-1,8,7,6,-1,11,10,9,-1};
	 Value *permute_mask_value = _mm256_set_epi32(gen,builder,permute_mask);
	 Value *shuffle_mask_value = _mm256_set_epi8(gen,builder,shuffle_mask);
	 int one_array[]{1,1,1,1,1,1,1,1};
	 int zero_array[]{0,0,0,0,0,0,0,0};
	 Value *one = _mm256_set_epi32(gen,builder,one_array);
	 Value *zero = _mm256_set_epi32(gen,builder,zero_array);

	{

		Value *pos_ptr = builder.CreateAlloca(builder.getInt32Ty());
		builder.CreateAlignedStore(builder.getInt32(0),pos_ptr,8);
		BasicBlock *label_for_cond = BasicBlock::Create(gen->context(),"for.cond",func);
		BasicBlock *label_for_body = BasicBlock::Create(gen->context(),"for.body",func);
		BasicBlock *label_for_end = BasicBlock::Create(gen->context(),"for.end",func);
		builder.CreateBr(label_for_cond);

		builder.SetInsertPoint(label_for_cond);
		{
			Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
			//print(gen,builder,"%d\n",pos_value);
			Value *cmp = builder.CreateICmpULT(pos_value,builder.getInt32(80000000));
			builder.CreateCondBr(cmp,label_for_body,label_for_end);
		}
		builder.SetInsertPoint(label_for_body);
		{
			//permute byte
			Value *ptr_byte_32_8 = builder.CreateBitCast(byte_ptr, type_32_8_pointer);
			Value *byteGEP = builder.CreateGEP(type_32_8,ptr_byte_32_8,builder.getInt32(0));
			Value *byte_value = builder.CreateAlignedLoad(byteGEP,1);
			byte_value = _mm256_permutevar8x32_epi32(gen,builder,byte_value,permute_mask_value);
			byte_value = builder.CreateBitCast(byte_value,type_8_32);

			Value *shuffled_value = _mm256_shuffle_epi8(gen,builder,byte_value,shuffle_mask_value);
			//result[pos] = unzigzag
			shuffled_value = builder.CreateBitCast(shuffled_value,type_32_8);
			Value *result_value = unZigZag32(gen,builder,shuffled_value,zero,one);
			Value *resultGEP = builder.CreateGEP(type_32_8,result_ptr,builder.getInt64(0));
			builder.CreateAlignedStore(result_value,resultGEP,16);

			Value *pos_value = builder.CreateAlignedLoad(pos_ptr,8);
			pos_value = builder.CreateAdd(pos_value,builder.getInt32(8));
			builder.CreateAlignedStore(pos_value,pos_ptr,8);
			builder.CreateBr(label_for_cond);
		}

		builder.SetInsertPoint(label_for_end);
		builder.CreateRetVoid();
	}

	 return func;
}

int main(int argc, char **argv){

	orc::parseGlobalFlag(argc,argv);
	unsigned char byte[32];
	byte[0] = (unsigned char)20;
	byte[1] = (unsigned char)113;
	byte[2] = (unsigned char)250;
	byte[3] = (unsigned char)10;
	byte[4] = (unsigned char)195;
	byte[5] = (unsigned char)172;
	byte[6] = (unsigned char)7;
	byte[7] = (unsigned char)161;
	byte[8] = (unsigned char)40;
	byte[9] = (unsigned char)55;
	byte[10] = (unsigned char)25;
	byte[11] = (unsigned char)72;
	byte[12] = (unsigned char)19;
	byte[13] = (unsigned char)1;
	byte[14] = (unsigned char)14;
	byte[15] = (unsigned char)1;
	byte[16] = (unsigned char)36;
	byte[17] = (unsigned char)154;
	byte[18] = (unsigned char)44;
	byte[19] = (unsigned char)173;
	byte[20] = (unsigned char)130;
	byte[21] = (unsigned char)44;
	byte[22] = (unsigned char)164;
	byte[23] = (unsigned char)92;
	byte[24] = (unsigned char)54;
	byte[25] = (unsigned char)25;
	byte[26] = (unsigned char)154;
	byte[27] = (unsigned char)17;
	byte[28] = (unsigned char)233;
	byte[29] = (unsigned char)80;
	byte[30] = (unsigned char)14;
	byte[31] = (unsigned char)1;

	unsigned int result[8];
	ObjectPool pool;
	LlvmCodeGen *gen = new LlvmCodeGen(&pool,"testglobal");
	initializeVectorization(gen);
	Function *fn = getfunc256(gen);


	gen->AddFunctionToJit(fn, (void **)&fn);
	gen->FinalizeModule();

	void (*getf)(unsigned int *, unsigned char *) = (void (*)( unsigned int* , unsigned char *))gen->GetFunction("test");

	getf(result, byte);

	for(int i=0;i<8;i++){
		cout<<result[i]<<endl;
	}




	return 0;

}

