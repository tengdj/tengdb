
#include "vectorization-ir.h"

using namespace llvm;

namespace orc{

void initializeVectorization(LlvmCodeGen *gen){
	 VectorType* type_8_16 = VectorType::get(IntegerType::get(gen->context(), 8), 16);
	 VectorType* type_8_32 = VectorType::get(IntegerType::get(gen->context(), 8), 32);
	 VectorType* type_32_8 = VectorType::get(IntegerType::get(gen->context(), 32), 8);

	{
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
	}
	{

		 std::vector<Type*> args;
		 args.push_back(type_8_32);
		 args.push_back(type_8_32);
		 FunctionType* func_type = FunctionType::get(
		  /*Result=*/type_8_32,
		  /*Params=*/args,
		  /*isVarArg=*/false);

		 Function* func_shuffle = gen->module()->getFunction("llvm.x86.avx2.pshuf.b");
		 if (!func_shuffle) {
			 func_shuffle = Function::Create(
		  /*Type=*/func_type,
		  /*Linkage=*/GlobalValue::ExternalLinkage,
		  /*Name=*/"llvm.x86.avx2.pshuf.b", gen->module()); // (external, no body)
			 func_shuffle->setCallingConv(CallingConv::C);
		 }
	}
	{

		 std::vector<Type*> args;
		 args.push_back(type_32_8);
		 args.push_back(type_32_8);
		 FunctionType* func_type = FunctionType::get(
		  /*Result=*/type_32_8,
		  /*Params=*/args,
		  /*isVarArg=*/false);

		 Function* func_shuffle = gen->module()->getFunction("llvm.x86.avx2.permd");
		 if (!func_shuffle) {
			 func_shuffle = Function::Create(
		  /*Type=*/func_type,
		  /*Linkage=*/GlobalValue::ExternalLinkage,
		  /*Name=*/"llvm.x86.avx2.permd", gen->module()); // (external, no body)
			 func_shuffle->setCallingConv(CallingConv::C);
		 }
	}
	{

		 std::vector<Type*>args;
		 args.push_back(type_32_8);
		 args.push_back(IntegerType::get(gen->context(), 32));
		 FunctionType* func_type = FunctionType::get(
		  /*Result=*/type_32_8,
		  /*Params=*/args,
		  /*isVarArg=*/false);
		 Function* func = gen->module()->getFunction("llvm.x86.avx2.psrai.d");
		 if (!func) {
			 func = Function::Create(
		  /*Type=*/func_type,
		  /*Linkage=*/GlobalValue::ExternalLinkage,
		  /*Name=*/"llvm.x86.avx2.psrai.d", gen->module()); // (external, no body)
		 func->setCallingConv(CallingConv::C);
		 }
	}

	{
		std::vector<Constant *> ones;
		std::vector<Constant *> zeros;
		for(int i=0;i<8;i++){
			ones.push_back(gen->getConstant(32,1));
			zeros.push_back(gen->getConstant(32,0));
		}

		Constant *one = ConstantVector::get(ones);
		Constant *zero = ConstantVector::get(zeros);

		GlobalVariable *one32 = new GlobalVariable(*(gen->module()),type_32_8,true,GlobalValue::ExternalLinkage,one,"one32");
		one32->setAlignment(16);
		GlobalVariable *zero32 = new GlobalVariable(*(gen->module()),type_32_8,true,GlobalValue::ExternalLinkage,zero,"zero32");
		zero32->setAlignment(16);
	}



}

Value *unZigZag32(LlvmCodeGen *gen, IRBuilder<> &builder, Value *data, Value *zero, Value *one){

	Value *and_value = builder.CreateAnd(data,one);
	Value *neg_value = builder.CreateNeg(and_value);
	Function *shift_func = gen->module()->getFunction("llvm.x86.avx2.psrai.d");
	Value *params[]{data,builder.getInt32(1)};
	Value *shift_value = builder.CreateCall(shift_func,params);
	return builder.CreateXor(shift_value,neg_value);
}

void vec_unZigZag16(LlvmCodeGen *gen, IRBuilder<> &builder, Value *data){
	;
}

Value *_mm_shuffle_epi8(LlvmCodeGen *gen, IRBuilder<> &builder, Value *byte_value, Value *mask_value){

	Function* func_shuffle = gen->module()->getFunction("llvm.x86.ssse3.pshuf.b.128");
	std::vector<Value*> params_shuffle;
	params_shuffle.push_back(byte_value);
	params_shuffle.push_back(mask_value);
	return builder.CreateCall(func_shuffle, params_shuffle);
}

Value *_mm256_shuffle_epi8(LlvmCodeGen *gen, IRBuilder<> &builder, Value *byte_value, Value *mask_value){

	Function* func_shuffle = gen->module()->getFunction("llvm.x86.avx2.pshuf.b");
	std::vector<Value*> params_shuffle;
	params_shuffle.push_back(byte_value);
	params_shuffle.push_back(mask_value);
	return builder.CreateCall(func_shuffle, params_shuffle);
}

Value *_mm256_permutevar8x32_epi32(LlvmCodeGen *gen, IRBuilder<> &builder, Value *byte_value, Value *mask_value){

	Function* func_shuffle = gen->module()->getFunction("llvm.x86.avx2.permd");
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

	Value *packed_value = builder.CreateInsertElement(UndefValue::get(type),builder.getInt32(value[0]),builder.getInt32(0));
	for(int i=1;i<8;i++){
		packed_value = builder.CreateInsertElement(packed_value,builder.getInt32(value[i]),builder.getInt32(i));
	}
	return packed_value;
}

}
