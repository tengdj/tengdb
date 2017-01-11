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

using namespace llvm;
using namespace std;
using namespace orc;

Value *_mm_shuffle_epi8(LlvmCodeGen *gen, IRBuilder<> &builder, Value *byte_value, Value *mask_value){
	 VectorType* type_8_16 = VectorType::get(IntegerType::get(gen->context(), 8), 16);

	 std::vector<Type*>FuncTy_12_args;
	 FuncTy_12_args.push_back(type_8_16);
	 FuncTy_12_args.push_back(type_8_16);
	 FunctionType* FuncTy_12 = FunctionType::get(
	  /*Result=*/type_8_16,
	  /*Params=*/FuncTy_12_args,
	  /*isVarArg=*/false);

	 Function* func_shuffle = gen->module()->getFunction("llvm.x86.ssse3.pshuf.b.128");
	 if (!func_shuffle) {
		 func_shuffle = Function::Create(
	  /*Type=*/FuncTy_12,
	  /*Linkage=*/GlobalValue::ExternalLinkage,
	  /*Name=*/"llvm.x86.ssse3.pshuf.b.128", gen->module()); // (external, no body)
		 func_shuffle->setCallingConv(CallingConv::C);
	 }


	  std::vector<Value*> params_shuffle;
	  params_shuffle.push_back(byte_value);
	  params_shuffle.push_back(mask_value);
	  return builder.CreateCall(func_shuffle, params_shuffle);
}

Value *_mm256_shuffle_epi8(LlvmCodeGen *gen, IRBuilder<> &builder, Value *byte_value, Value *mask_value){
	 VectorType* type_8_32 = VectorType::get(IntegerType::get(gen->context(), 8), 32);

	 std::vector<Type*> args;
	 args.push_back(type_8_32);
	 args.push_back(type_8_32);
	 FunctionType* func_type = FunctionType::get(
	  /*Result=*/type_8_32,
	  /*Params=*/args,
	  /*isVarArg=*/false);

	 Function* func_shuffle = gen->module()->getFunction("llvm.x86.ssse3.pshuf.b.128");
	 if (!func_shuffle) {
		 func_shuffle = Function::Create(
	  /*Type=*/func_type,
	  /*Linkage=*/GlobalValue::ExternalLinkage,
	  /*Name=*/"llvm.x86.ssse3.pshuf.b.128", gen->module()); // (external, no body)
		 func_shuffle->setCallingConv(CallingConv::C);
	 }


	  std::vector<Value*> params_shuffle;
	  params_shuffle.push_back(byte_value);
	  params_shuffle.push_back(mask_value);
	  return builder.CreateCall(func_shuffle, params_shuffle);
}


Value *_mm_set_epi8(LlvmCodeGen *gen,IRBuilder<> &builder, int value[]){

	VectorType* type_8_16 = VectorType::get(IntegerType::get(gen->context(), 8), 16);

	Value *packed_value = builder.CreateInsertElement(UndefValue::get(type_8_16),builder.getInt8(value[0]),builder.getInt32(0));
	for(int i=1;i<16;i++){
		packed_value = builder.CreateInsertElement(packed_value,builder.getInt8(value[i]),builder.getInt32(i));
	}
	return packed_value;
}

Value *_mm256_set_epi8(LlvmCodeGen *gen,IRBuilder<> &builder, int value[]){
	VectorType* type = VectorType::get(IntegerType::get(gen->context(), 8), 32);

	Value *packed_value = builder.CreateInsertElement(UndefValue::get(type),builder.getInt8(value[0]),builder.getInt32(0));
	for(int i=1;i<32;i++){
		packed_value = builder.CreateInsertElement(packed_value,builder.getInt8(value[i]),builder.getInt32(i));
	}
	return packed_value;
}

Value *_mm256_set_epi32(LlvmCodeGen *gen,IRBuilder<> &builder, int value[]){
	VectorType* type = VectorType::get(IntegerType::get(gen->context(), 32), 8);

	Value *packed_value = builder.CreateInsertElement(UndefValue::get(type),builder.getInt8(value[0]),builder.getInt32(0));
	for(int i=1;i<8;i++){
		packed_value = builder.CreateInsertElement(packed_value,builder.getInt8(value[i]),builder.getInt32(i));
	}
	return packed_value;
}

Function *getfunc(LlvmCodeGen *gen){
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

	unsigned int result[4];
	ObjectPool pool;
	LlvmCodeGen *gen = new LlvmCodeGen(&pool,"testglobal");
	Function *fn = getfunc(gen);


	gen->AddFunctionToJit(fn, (void **)&fn);
	gen->FinalizeModule();

	void (*getf)(unsigned int *, unsigned char *) = (void (*)( unsigned int* , unsigned char *))gen->GetFunction("test");

	getf(result, byte);

	for(int i=0;i<4;i++){
		cout<<result[i]<<endl;
	}




	return 0;

}

