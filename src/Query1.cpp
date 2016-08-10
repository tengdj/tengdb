/*
 * Query1.cpp
 *
 *  Created on: Jul 25, 2016
 *      Author: teng
 */

#include <iostream>
#include <stdlib.h>
#include <memory>
#include <string>
#include <list>
#include "llvm-codegen.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "../include/config.h"
#include "../include/Reader.h"
#include "../include/RLE2.h"
#include "../include/util.h"
#include "functions-ir.h"
#include "functions.h"
#include <dirent.h>

#include "../include/MemoryPool.h"

using namespace std;
using namespace orc;
using namespace llvm;

/*
 *
 *
 *
select
      l_returnflag,
      l_linestatus,
	  sum(l_quantity),
      sum(l_extendedprice),
      sum(l_extendedprice * (1 - l_discount)),
      sum(l_extendedprice * (1 - l_discount) * (1 + l_tax)),
      avg(l_quantity),
      avg(l_extendedprice),
      avg(l_discount),
      count(1)
from
lineitem
where
     l_shipdate<='1998-09-02'
group by
     l_returnflag,
     l_linestatus;
 *
 * */
std::map<std::string,int> colmap;
void init(){
	colmap["l_orderkey"] =  1;
	colmap["l_partkey"] = 2;
	colmap["l_suppkey"] = 3;
	colmap["l_linenumber"] = 4;
	colmap["l_quantity"] = 5;
	colmap["l_extendedprice"] = 6;
	colmap["l_discount"] = 7;
	colmap["l_tax"] = 8;
	colmap["l_returnflag"] = 9;
	colmap["l_linestatus"] = 10;
	colmap["l_shipdate"] = 11;
	colmap["l_commitdate"] = 12;
	colmap["l_receiptdate"] = 13;
	colmap["l_shipinstruct"] = 14;
	colmap["l_shipmode"] = 15;
	colmap["l_comment"] = 16;
}

void init1(){
	colmap["l_quantity"] = 1;
	colmap["l_extendedprice"] = 2;
	colmap["l_discount"] = 3;
	colmap["l_tax"] = 4;
	colmap["l_returnflag"] = 5;
	colmap["l_linestatus"] = 6;
	colmap["l_shipdate"] = 7;
}

Function *genfunc_hash(LlvmCodeGen *gen){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"hash",builder.getInt64Ty(),true);
	proto.AddArgument("offset", gen->int_type(64));
	proto.AddArgument("GPresult", PointerType::get(PointerType::get(builder.getInt64Ty(),0),0));

	llvm::Value *params[2];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *offset_value = params[0];
	Value *GPresult_addr = params[1];

	Value *GPresult_addrGEP0 = builder.CreateGEP(GPresult_addr, builder.getInt64(0));
	Value *GPresult_addrGEP1 = builder.CreateGEP(GPresult_addr, builder.getInt64(1));

	Value *return_addr = builder.CreateAlignedLoad(GPresult_addrGEP0,8);
	Value *status_addr = builder.CreateAlignedLoad(GPresult_addrGEP1,8);

	Value *return_addrGEP = builder.CreateGEP(return_addr, offset_value);
	Value *status_addrGEP = builder.CreateGEP(status_addr, offset_value);

	Value *return_value = builder.CreateAlignedLoad(return_addrGEP,8);
	Value *status_value = builder.CreateAlignedLoad(status_addrGEP,8);
	Value *mul_value = builder.CreateMul(return_value,builder.getInt64(2));

	Value *add_value = builder.CreateAdd(mul_value,status_value);
	builder.CreateRet(add_value);

	return fn;
}

bool process = true;
Function *genfunc_process_quantity(LlvmCodeGen *gen){

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

	if(process)
	{
		//aggresult[aggpos][0]+rowvalue
		Value *hashresult_ptr = orc::MemSpace::getField(gen,builder,space_ptr,MemSpaceField::hashResult);
		Value *aggpos_GEP = builder.CreateGEP(builder.getInt64Ty(),hashresult_ptr,offset_value);
		Value *aggpos_value = builder.CreateAlignedLoad(aggpos_GEP,8);
		Value *aggresult_ptr = MemSpace::getField(gen,builder,space_ptr,MemSpaceField::doubleAggResult);

		Value *aggresult_GEP = builder.CreateGEP(gen->double_ptr_type(),aggresult_ptr,aggpos_value);
		aggresult_ptr = builder.CreateAlignedLoad(aggresult_GEP,8);
		aggresult_GEP = builder.CreateGEP(gen->double_type(),aggresult_ptr,builder.getInt64(0));
		Value *aggresult_value = builder.CreateLoad(aggresult_GEP);
		Value *add_value = builder.CreateFAdd(aggresult_value,rowvalue_value);
		builder.CreateStore(add_value,aggresult_GEP);
	}

	builder.CreateRetVoid();

	return fn;
}
Function *genfunc_process_price(LlvmCodeGen *gen){
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

	if(process)
	{
		//save result
		Value *result_ptr = orc::MemSpace::getField(gen,builder,space_ptr,MemSpaceField::doubleResult);
		Value *result_GEP = builder.CreateGEP(gen->double_ptr_type(),result_ptr,builder.getInt64(0));
		result_ptr = builder.CreateAlignedLoad(result_GEP,8);
		result_GEP = builder.CreateGEP(gen->double_type(),result_ptr,offset_value);
		builder.CreateStore(rowvalue_value,result_GEP);

		//aggregation
		Value *hashresult_ptr = orc::MemSpace::getField(gen,builder,space_ptr,MemSpaceField::hashResult);
		Value *aggpos_GEP = builder.CreateGEP(builder.getInt64Ty(),hashresult_ptr,offset_value);
		Value *aggpos_value = builder.CreateAlignedLoad(aggpos_GEP,8);
		Value *aggresult_ptr = MemSpace::getField(gen,builder,space_ptr,MemSpaceField::doubleAggResult);

		Value *aggresult_GEP = builder.CreateGEP(gen->double_ptr_type(),aggresult_ptr,aggpos_value);
		aggresult_ptr = builder.CreateAlignedLoad(aggresult_GEP,8);
		aggresult_GEP = builder.CreateGEP(gen->double_type(),aggresult_ptr,builder.getInt64(1));
		Value *aggresult_value = builder.CreateLoad(aggresult_GEP);
		Value *add_value = builder.CreateFAdd(aggresult_value,rowvalue_value);
		builder.CreateStore(add_value,aggresult_GEP);
	}
	builder.CreateRetVoid();

	return fn;
}

Function *genfunc_process_discount(LlvmCodeGen *gen){
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

	if(process)
	{
		Value *result_ptr = orc::MemSpace::getField(gen,builder,space_ptr,MemSpaceField::doubleResult);
		Value *result_GEP = builder.CreateGEP(gen->double_ptr_type(),result_ptr,builder.getInt64(0));
		result_ptr = builder.CreateAlignedLoad(result_GEP,8);
		result_GEP = builder.CreateGEP(gen->double_type(),result_ptr,offset_value);
		Value *price_value = builder.CreateLoad(result_GEP);

		Value *hashresult_ptr = orc::MemSpace::getField(gen,builder,space_ptr,MemSpaceField::hashResult);
		Value *aggpos_GEP = builder.CreateGEP(builder.getInt64Ty(),hashresult_ptr,offset_value);
		Value *aggpos_value = builder.CreateAlignedLoad(aggpos_GEP,8);
		Value *aggresult_ptr = MemSpace::getField(gen,builder,space_ptr,MemSpaceField::doubleAggResult);
		Value *aggresult_GEP = builder.CreateGEP(gen->double_ptr_type(),aggresult_ptr,aggpos_value);
		aggresult_ptr = builder.CreateAlignedLoad(aggresult_GEP,8);

		//sum(price*(1-discount))
		Value *minus_value = builder.CreateFSub(ConstantFP::get(gen->double_type(),1),rowvalue_value);
		Value *mul_value = builder.CreateFMul(price_value,minus_value);
		//for sum(price*(1-discount)*(1+tax))
		builder.CreateStore(mul_value,result_GEP);

		aggresult_GEP = builder.CreateGEP(gen->double_type(),aggresult_ptr,builder.getInt64(3));
		Value *aggresult_value = builder.CreateLoad(aggresult_GEP);
		Value *add_value = builder.CreateFAdd(aggresult_value,mul_value);
		builder.CreateStore(add_value,aggresult_GEP);

		//sum(discount)
		aggresult_GEP = builder.CreateGEP(gen->double_type(),aggresult_ptr,builder.getInt64(2));
		aggresult_value = builder.CreateLoad(aggresult_GEP);
		add_value = builder.CreateFAdd(aggresult_value,rowvalue_value);
		builder.CreateStore(add_value,aggresult_GEP);
	}

	builder.CreateRetVoid();
	return fn;
}

Function *genfunc_process_tax(LlvmCodeGen *gen){
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


	if(process)
	{

		Value *hashresult_ptr = orc::MemSpace::getField(gen,builder,space_ptr,MemSpaceField::hashResult);
		Value *aggpos_GEP = builder.CreateGEP(builder.getInt64Ty(),hashresult_ptr,offset_value);
		Value *aggpos_value = builder.CreateAlignedLoad(aggpos_GEP,8);
		Value *aggresult_ptr = MemSpace::getField(gen,builder,space_ptr,MemSpaceField::doubleAggResult);
		Value *aggresult_GEP = builder.CreateGEP(gen->double_ptr_type(),aggresult_ptr,aggpos_value);
		aggresult_ptr = builder.CreateAlignedLoad(aggresult_GEP,8);

		//price*(1-discount)*(1+tax)
		Value *result_ptr = orc::MemSpace::getField(gen,builder,space_ptr,MemSpaceField::doubleResult);
		Value *result_GEP = builder.CreateGEP(gen->double_ptr_type(),result_ptr,builder.getInt64(0));
		result_ptr = builder.CreateAlignedLoad(result_GEP,8);
		result_GEP = builder.CreateGEP(gen->double_type(),result_ptr,offset_value);
		Value *priceTimesdiscount_value = builder.CreateLoad(result_GEP);
		Value *plus_value = builder.CreateFAdd(ConstantFP::get(gen->double_type(),1),rowvalue_value);
		Value *mul_value = builder.CreateFMul(priceTimesdiscount_value,plus_value);

		aggresult_GEP = builder.CreateGEP(gen->double_type(),aggresult_ptr,builder.getInt64(4));
		Value *aggresult_value = builder.CreateLoad(aggresult_GEP);
		Value *add_value = builder.CreateFAdd(aggresult_value,mul_value);
		builder.CreateStore(add_value,aggresult_GEP);
	}
	builder.CreateRetVoid();

	return fn;
}

int  main2(int argc, char **args){
	init();

	string filepath = "/home/teng/orcfile/all/000000_0";
	string folderpath = "/home/teng/orcfile/all/";
	ReaderOptions opts;

	ObjectPool pool;
	LlvmCodeGen *gen;
	MemoryPool *mempool = getDefaultPool();
	DataBuffer<unsigned char> buf(*mempool,100);

	bool local = argc>1;
	if(local){
		orc::LlvmCodeGen::LoadFromFile(&pool,"./query1_opt.ll","query1",&gen);
	}else{

		std::unique_ptr<orc::Reader> reader = orc::createReader(orc::readLocalFile(filepath),opts);
		gen = new LlvmCodeGen(&pool,"query1");

		//gen func for group columns
		ColumnInfo cinfo_return;
		ColumnInfo cinfo_status;
		cinfo_return.colname = "l_returnflag";
		cinfo_status.colname = "l_linestatus";
		for(int i=0;i<reader->getStripeSize();i++){
			uint64_t datasize;
			reader->readdata(i,colmap["l_returnflag"],proto::Stream_Kind_DATA, buf,datasize);
			RLE2 *rle_return = new RLE2(buf.data(), datasize, false);
			rle_return->getInfo(cinfo_return);
			delete rle_return;

			reader->readdata(i,colmap["l_linestatus"],proto::Stream_Kind_DATA, buf,datasize);
			RLE2 *rle_status = new RLE2(buf.data(), datasize, false);
			rle_status->getInfo(cinfo_status);
			delete rle_status;
		}
		genfunc_next(gen, cinfo_return);
		genfunc_next(gen, cinfo_status);

		//gen func for select columns
		int numreplaced;
		Function *readDouble_func = genfunc_readDouble(gen);

		//function for quantity
		ColumnInfo quantity_info;
		quantity_info.colname = "quantity";
		quantity_info.updateAggregation = true;
		Function *quantity_fn = gen->ReplaceCallSites(readDouble_func,false,genfunc_process_quantity(gen),"process",&numreplaced);
		quantity_fn->setName("nextFunc_quantity");

		//function for price
		ColumnInfo price_info;
		price_info.colname = "price";
		price_info.saveResult = true;
		price_info.updateAggregation = true;
		Function *price_fn = gen->ReplaceCallSites(readDouble_func,false,genfunc_process_price(gen),"process",&numreplaced);
		price_fn->setName("nextFunc_price");
		//function for discount
		ColumnInfo discount_info;
		discount_info.colname = "discount";
		discount_info.updateAggregation = true;
		Function *discount_fn = gen->ReplaceCallSites(readDouble_func,false,genfunc_process_discount(gen),"process",&numreplaced);
		discount_fn->setName("nextFunc_discount");
		//function for tax
		ColumnInfo tax_info;
		tax_info.colname = "tax";
		tax_info.updateAggregation = true;
		Function *tax_fn = gen->ReplaceCallSites(readDouble_func,false,genfunc_process_tax(gen),"process",&numreplaced);
		tax_fn->setName("nextFunc_tax");

		//gen->EnableOptimizations(false);
		gen->setOptimizationLevel(3);
		gen->FinalizeModule();

	}



	//generate function for RLE
	int64_t (*next_return)(unsigned char *, uint64_t &, uint64_t, MemSpace &space, int64_t *)
			= (int64_t (*)(unsigned char *, uint64_t &, uint64_t, MemSpace &space, int64_t *iresult))gen->GetFunction("nextFunc_l_returnflag");
	int64_t (*next_status)(unsigned char *, uint64_t &, uint64_t, MemSpace &space, int64_t *)
			= (int64_t (*)(unsigned char *, uint64_t &, uint64_t, MemSpace &space, int64_t *iresult))gen->GetFunction("nextFunc_l_linestatus");;

	//generate function for doubles
	void (*next_quantity)(unsigned char* data, uint64_t &index, uint64_t offset,uint64_t length, MemSpace &)
			= 	(void (*)(unsigned char*, uint64_t &, uint64_t , uint64_t , MemSpace &))gen->GetFunction("nextFunc_quantity");
	void (*next_price)(unsigned char* data, uint64_t &index, uint64_t offset,uint64_t length, MemSpace &)
			= 	(void (*)(unsigned char*, uint64_t &, uint64_t , uint64_t , MemSpace &))gen->GetFunction("nextFunc_price");
	void (*next_discount)(unsigned char* data, uint64_t &index, uint64_t offset,uint64_t length, MemSpace &)
			= 	(void (*)(unsigned char*, uint64_t &, uint64_t , uint64_t , MemSpace &))gen->GetFunction("nextFunc_discount");
	void (*next_tax)(unsigned char* data, uint64_t &index, uint64_t offset,uint64_t length, MemSpace &)
			= 	(void (*)(unsigned char*, uint64_t &, uint64_t , uint64_t , MemSpace &))gen->GetFunction("nextFunc_tax");
	//exit(0);
/*
 *
 * 	proto.AddArgument("data", gen->int_ptr_type(8));
	proto.AddArgument("result", gen->double_ptr_type());
	proto.AddArgument("index", gen->int_ptr_type(64));
	proto.AddArgument("offset",gen->int_type(64));
	proto.AddArgument("length",gen->int_type(64));
	proto.AddArgument("bitmap",builder.getInt8PtrTy());
	proto.AddArgument("IRresult",double_ptr);
	proto.AddArgument("GPresult",int_ptr);
	proto.AddArgument("aggresult",double_ptr);
 *
 * */

	//0: sum quantity
	//1: sum price
	//2: sum discount
	//3: sum price*(1-discount)
	//4: sum price*(1-discount)*(1+tax)
	double *aggresult[6];
	aggresult[0] = new double[5];
	aggresult[1] = new double[5];
	aggresult[2] = new double[5];
	aggresult[3] = new double[5];
	aggresult[4] = new double[5];
	aggresult[5] = new double[5];

	for(int i=0;i<6;i++){
		for(int j=0;j<5;j++){
			aggresult[i][j] = 0;
		}
	}

	DataBuffer<char> bitmap_buf(*mempool,100);
	DataBuffer<int64_t> return_buf(*mempool,100);
	DataBuffer<int64_t> status_buf(*mempool,100);
	DataBuffer<uint64_t> hashresult_buf(*mempool,100);
	DataBuffer<double> iresult_buf(*mempool,100);

	int round = 102400;
	DIR *dir;
	struct dirent *ent;
	uint64_t sum;
	MemSpace space;
	//space.bitMap = bitmap_buf.data();
	double *IRresult[1];
	space.doubleAggResult = aggresult;
	space.hashResult = hashresult_buf.data();
	space.doubleResult = IRresult;

	if ((dir = opendir (folderpath.c_str())) != NULL) {
	  /* print all the files and directories within directory */
	  while ((ent = readdir (dir)) != NULL) {
		  if(ent->d_type==DT_REG){
			std::unique_ptr<orc::Reader> reader = orc::createReader(orc::readLocalFile(folderpath+ent->d_name),opts);
			for(int i=0;i<reader->getStripeSize();i++){

				proto::StripeInformation sinfo = reader->getStrips(i);
				uint64_t nrows = sinfo.numberofrows();

				if(bitmap_buf.capacity()<nrows){
					bitmap_buf.resize(nrows);
					return_buf.resize(nrows);
					status_buf.resize(nrows);
					hashresult_buf.resize(nrows);
					iresult_buf.resize(nrows);
					space.hashResult = hashresult_buf.data();
				}
				IRresult[0] = iresult_buf.data();

				//parse returnflag
				{
					uint64_t datasize = 0;
					reader->readdata(i,colmap["l_returnflag"],proto::Stream_Kind_DATA, buf,datasize);
					uint64_t index=0;
					uint64_t offset = 0;
					while(index<datasize){
						offset += next_return(buf.data(),index,offset,space,return_buf.data());
					}
				}
				//parse linestatus
				{
					uint64_t datasize = 0;
					reader->readdata(i,colmap["l_linestatus"],proto::Stream_Kind_DATA, buf,datasize);

					uint64_t index=0;
					uint64_t offset = 0;
					while(index<datasize){
						offset += next_status(buf.data(),index,offset,space,status_buf.data());
					}
				}

				for(uint64_t j=0;j<nrows;j++){
					hashresult_buf[j] = return_buf[j]*2+status_buf[j];
				}


				//next quantity
				{
					uint64_t datasize = 0;
					reader->readdata(i,colmap["l_quantity"],proto::Stream_Kind_DATA, buf,datasize);

					uint64_t index=0;
					uint64_t offset = 0;
					for(;offset<nrows;offset+=round){
						next_quantity(buf.data(),index, offset,(nrows-offset)>round?round:(nrows-offset),space);
					}
				}

				//next price
				{
					uint64_t datasize = 0;
					reader->readdata(i,colmap["l_extendedprice"],proto::Stream_Kind_DATA, buf,datasize);

					uint64_t index=0;
					uint64_t offset = 0;
					for(;offset<nrows;offset+=round){
						next_price(buf.data(),index, offset,(nrows-offset)>round?round:(nrows-offset),space);
					}
				}

				//next discount
				{
					uint64_t datasize = 0;
					reader->readdata(i,colmap["l_discount"],proto::Stream_Kind_DATA, buf,datasize);

					uint64_t index=0;
					uint64_t offset = 0;
					for(;offset<nrows;offset+=round){
						next_discount(buf.data(),index, offset,(nrows-offset)>round?round:(nrows-offset),space);
					}
				}

				//next tax
				{
					uint64_t datasize = 0;
					reader->readdata(i,colmap["l_tax"],proto::Stream_Kind_DATA, buf,datasize);

					uint64_t index=0;
					uint64_t offset = 0;
					for(;offset<nrows;offset+=round){
						next_tax(buf.data(),index, offset,(nrows-offset)>round?round:(nrows-offset),space);
					}
				}

				//delete IRresult[0];
			}
		  }
	  }
	  closedir (dir);
	} else {
	  /* could not open directory */
	  perror ("");
	  return EXIT_FAILURE;
	}

	for(int i=0;i<6;i++){
		cout<<i;
		for(int j=0;j<5;j++){
			cout<<" "<<aggresult[i][j];
		}
		cout<<endl;
	}
	return 0;
}

int  main(int argc, char **args){

	proto::Stream_Kind kind;
	string filepath;
	bool issigned;
	int column = 1;
	string colname;

	filepath = "/home/teng/orcfile/testdirect/000000_0";kind = proto::Stream_Kind_DATA;issigned=true;colname="direct";;
	//filepath = "/home/teng/orcfile/testrepeat/000000_0";kind = proto::Stream_Kind_DATA;issigned=true;colname="repeat";
	//filepath = "/home/teng/orcfile/testdelta/000000_0";kind = proto::Stream_Kind_DATA;issigned=true;colname="delta";
	//filepath = "/home/teng/orcfile/testdecimal/000000_0";kind = proto::Stream_Kind_SECONDARY;issigned=false;colname="decimal";
	//filepath = "/home/teng/orcfile/testdictionary/000000_0";kind = proto::Stream_Kind_DATA;issigned = false;colname="dictionary";
	//filepath = "/home/teng/orcfile/all/000000_0";kind = proto::Stream_Kind_DATA;issigned = false;column = 10;colname = "l_partkey";


	//filepath = "/home/teng/orcfile/testmix/000000_0";kind = proto::Stream_Kind_LENGTH;
	ReaderOptions opts;
	std::unique_ptr<orc::Reader> reader = orc::createReader(orc::readLocalFile(filepath),opts);

	ObjectPool pool;
	LlvmCodeGen *gen;
	MemoryPool *mempool = getDefaultPool();
	DataBuffer<unsigned char> buf(*mempool,100);
	bool local = argc>1;
	if(local){
		orc::LlvmCodeGen::LoadFromFile(&pool,"./testutil_opt.ll","testutil",&gen);
	}else{
		gen = new LlvmCodeGen(&pool,"testutil");
		ColumnInfo cinfo;
		cinfo.colname = colname;
		for(int i=0;i<reader->getStripeSize();i++){
			uint64_t datasize;
			reader->readdata(i,column,kind, buf,datasize);
			RLE2 *rle = new RLE2(buf.data(), datasize, issigned);
			rle->getInfo(cinfo);
			delete rle;
		}

		cout<<cinfo.hasrepeat<<" "<<cinfo.repeatInfo.isbitwidthfixed<<" "<<cinfo.repeatInfo.bitwidth<<endl;
		cout<<cinfo.hasdirect<<" "<<cinfo.directInfo.isbitwidthfixed<<" "<<cinfo.directInfo.bitwidth<<endl;
		cout<<cinfo.hasdelta<<" "<<cinfo.deltaInfo.isbitwidthfixed<<" "<<cinfo.deltaInfo.bitwidth<<endl;
		genfunc_next(gen, cinfo);

		gen->setOptimizationLevel(3);
		gen->FinalizeModule();
	}

	uint64_t funcaddr = gen->GetFunction("nextFunc_"+colname);
	int64_t (*next)(unsigned char *, uint64_t &, uint64_t, MemSpace &, int64_t *)
			= (int64_t (*)(unsigned char *, uint64_t &, uint64_t,MemSpace &, int64_t *))funcaddr;

	DataBuffer<int64_t> intbuf(*mempool,100);
	MemSpace space;
	for(int i=0;i<reader->getStripeSize();i++){

		uint64_t datasize;

		proto::StripeInformation sinfo = reader->getStrips(i);
		uint64_t nrows = sinfo.numberofrows();
		uint64_t index=0;
		uint64_t offset = 0;
		reader->readdata(i,column,kind, buf,datasize);

		if(intbuf.size()<nrows){
			intbuf.resize(nrows);
		}
		int64_t *result = intbuf.data();

		//cout<<static_cast<uint32_t>(data[0])<<" "<<static_cast<uint32_t>(data[1])<<std::endl;
		int count[2];
		count[0] = 0;
		count[1] = 0;

		while(index<datasize){
			uint64_t runlength;
			runlength = next(buf.data(),index,offset,space,result);
//			for(int r=offset;r<offset+runlength;r++){
//				cout<<r<<"	"<<result[r]<<endl;
//			}
			offset += runlength;
			//cout<<index<<endl;
			//cout<<runlength<<" "<<index<<endl;


			//*result += runlength*sizeof(uint64_t);
//			if(runlength!=512)

		}
		//cout<<count[0]<<" "<<count[1]<<endl;

	}

	return 0;
}
