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


Function *genfunc_process_quantity(LlvmCodeGen *gen){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"process",builder.getVoidTy(),true);
	Type *double_ptr = PointerType::get(PointerType::get(builder.getDoubleTy(),0),0);
	proto.AddArgument("rowvalue", gen->double_type());
	proto.AddArgument("IRresult",double_ptr);
	proto.AddArgument("aggresult",double_ptr);
	proto.AddArgument("offset",builder.getInt64Ty());
	proto.AddArgument("aggpos",builder.getInt64Ty());

	llvm::Value *params[5];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *rowvalue_value = params[0];
	Value *IRresult_addr = params[1];
	Value *aggresult_addr = params[2];
	Value *offset_value = params[3];
	Value *aggpos_value = params[4];

	Value *aggresult_addrGEP = builder.CreateGEP(gen->double_ptr_type(),aggresult_addr,aggpos_value);
	Value *aggresult_addr_addr = builder.CreateAlignedLoad(aggresult_addrGEP,8);
	Value *aggresult_addr_addrGEP = builder.CreateGEP(gen->double_type(),aggresult_addr_addr,builder.getInt64(0));
	Value *aggresult_value = builder.CreateLoad(aggresult_addr_addrGEP);
	Value *add_value = builder.CreateFAdd(aggresult_value,rowvalue_value);
	builder.CreateStore(add_value,aggresult_addr_addrGEP);

	builder.CreateRetVoid();

	return fn;
}
Function *genfunc_process_price(LlvmCodeGen *gen){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"process",builder.getVoidTy(),true);
	Type *double_ptr = PointerType::get(PointerType::get(builder.getDoubleTy(),0),0);
	proto.AddArgument("rowvalue", gen->double_type());
	proto.AddArgument("IRresult",double_ptr);
	proto.AddArgument("aggresult",double_ptr);
	proto.AddArgument("offset",builder.getInt64Ty());
	proto.AddArgument("aggpos",builder.getInt64Ty());

	llvm::Value *params[5];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *rowvalue_value = params[0];
	Value *IRresult_addr = params[1];
	Value *aggresult_addr = params[2];
	Value *offset_value = params[3];
	Value *aggpos_value = params[4];

	Value *aggresult_addrGEP = builder.CreateGEP(gen->double_ptr_type(),aggresult_addr,aggpos_value);
	Value *aggresult_addr_addr = builder.CreateAlignedLoad(aggresult_addrGEP,8);
	Value *aggresult_addr_addrGEP = builder.CreateGEP(gen->double_type(),aggresult_addr_addr,builder.getInt64(1));
	Value *aggresult_value = builder.CreateLoad(aggresult_addr_addrGEP);
	Value *add_value = builder.CreateFAdd(aggresult_value,rowvalue_value);
	builder.CreateStore(add_value,aggresult_addr_addrGEP);

	builder.CreateRetVoid();

	return fn;
}

//process(double rowvalue, double *IRresult, double *aggresult)

Function *genfunc_process_discount(LlvmCodeGen *gen){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"process",builder.getVoidTy(),true);
	Type *double_ptr = PointerType::get(PointerType::get(builder.getDoubleTy(),0),0);
	proto.AddArgument("rowvalue", gen->double_type());
	proto.AddArgument("IRresult",double_ptr);
	proto.AddArgument("aggresult",double_ptr);
	proto.AddArgument("offset",builder.getInt64Ty());
	proto.AddArgument("aggpos",builder.getInt64Ty());

	llvm::Value *params[5];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *rowvalue_value = params[0];
	Value *IRresult_addr = params[1];
	Value *aggresult_addr = params[2];
	Value *offset_value = params[3];
	Value *aggpos_value = params[4];

	Value *IRresult_addrGEP = builder.CreateGEP(gen->double_ptr_type(),IRresult_addr,builder.getInt64(0));
	Value *IRresult_addr_addr = builder.CreateAlignedLoad(IRresult_addrGEP,8);
	Value *IRresult_addr_addrGEP = builder.CreateGEP(gen->double_type(),IRresult_addr_addr,offset_value);
	Value *price_value = builder.CreateLoad(IRresult_addr_addrGEP);

	{
		//sum(price*(1-discount))
		Value *minus_value = builder.CreateFSub(ConstantFP::get(gen->double_type(),1),rowvalue_value);
		Value *mul_value = builder.CreateFMul(price_value,minus_value);
		Value *aggresult_addrGEP = builder.CreateGEP(gen->double_ptr_type(),aggresult_addr,aggpos_value);
		Value *aggresult_addr_addr = builder.CreateAlignedLoad(aggresult_addrGEP,8);
		Value *aggresult_addr_addrGEP = builder.CreateGEP(gen->double_type(),aggresult_addr_addr,builder.getInt64(3));
		Value *aggresult_value = builder.CreateLoad(aggresult_addr_addrGEP);
		Value *add_value = builder.CreateFAdd(aggresult_value,mul_value);
		builder.CreateStore(add_value,aggresult_addr_addrGEP);
		builder.CreateStore(mul_value,IRresult_addr_addrGEP);
		//sum(discount)
		Value *aggresult_addr_addrGEP_discount = builder.CreateGEP(gen->double_type(),aggresult_addr_addr,builder.getInt64(2));
		Value *aggresult_value_discount = builder.CreateLoad(aggresult_addr_addrGEP_discount);
		Value *add_value_discount = builder.CreateFAdd(aggresult_value_discount,rowvalue_value);
		builder.CreateStore(add_value_discount,aggresult_value_discount);
	}

	builder.CreateRetVoid();

	return fn;
}

Function *genfunc_process_tax(LlvmCodeGen *gen){
	IRBuilder<> builder(gen->context());
	LlvmCodeGen::FnPrototype proto(gen,"process",builder.getVoidTy(),true);
	Type *double_ptr = PointerType::get(PointerType::get(builder.getDoubleTy(),0),0);
	proto.AddArgument("rowvalue", gen->double_type());
	proto.AddArgument("IRresult",double_ptr);
	proto.AddArgument("aggresult",double_ptr);
	proto.AddArgument("offset",builder.getInt64Ty());
	proto.AddArgument("aggpos",builder.getInt64Ty());

	llvm::Value *params[5];
	Function *fn = proto.GeneratePrototype(&builder,params);

	Value *rowvalue_value = params[0];
	Value *IRresult_addr = params[1];
	Value *aggresult_addr = params[2];
	Value *offset_value = params[3];
	Value *aggpos_value = params[4];

	{
		Value *IRresult_addrGEP = builder.CreateGEP(gen->double_ptr_type(),IRresult_addr,builder.getInt64(0));
		Value *IRresult_addr_addr = builder.CreateAlignedLoad(IRresult_addrGEP,8);
		Value *IRresult_addr_addrGEP = builder.CreateGEP(gen->double_type(),IRresult_addr_addr,offset_value);
		Value *priceTimesdiscount_value = builder.CreateLoad(IRresult_addr_addrGEP);
		//price*(1-discount)*(1+tax)
		Value *plus_value = builder.CreateFAdd(ConstantFP::get(gen->double_type(),1),rowvalue_value);
		Value *mul_value = builder.CreateFMul(priceTimesdiscount_value,plus_value);

		Value *aggresult_addrGEP = builder.CreateGEP(gen->double_ptr_type(),aggresult_addr,aggpos_value);
		Value *aggresult_addr_addr = builder.CreateAlignedLoad(aggresult_addrGEP,8);
		Value *aggresult_addr_addrGEP = builder.CreateGEP(gen->double_type(),aggresult_addr_addr,builder.getInt64(4));
		Value *aggresult_value = builder.CreateLoad(aggresult_addr_addrGEP);
		Value *add_value = builder.CreateFAdd(aggresult_value,mul_value);
		builder.CreateStore(add_value,aggresult_addr_addrGEP);
	}
	builder.CreateRetVoid();

	return fn;
}
int  main(int argc, char **args){
	init();
	int col[2] = {colmap["l_returnflag"],colmap["l_linestatus"]};

	string filepath = "/home/teng/orcfile/all/000000_0";
	string folderpath = "/home/teng/orcfile/all/";
	ReaderOptions opts;

	ObjectPool pool;
	LlvmCodeGen *gen;
	bool local = argc>1;
	if(local){
		orc::LlvmCodeGen::LoadFromFile(&pool,"/home/teng/orcir/query1_opt.ll","query1",&gen);
	}else{

		std::unique_ptr<orc::Reader> reader = orc::createReader(orc::readLocalFile(filepath),opts);
		gen = new LlvmCodeGen(&pool,"query1");
		ColumnInfo cinfo[2];
		cinfo[0].colname = "l_returnflag";
		cinfo[1].colname = "l_linestatus";

		for(int i=0;i<reader->getStripeSize();i++){
			for(int t = 0;t<2;t++){
				unsigned char *data;
				uint64_t datasize;
				reader->readdata(i,col[t],proto::Stream_Kind_DATA, &data,&datasize);
				RLE2 *rle = new RLE2(data, datasize, false);
				rle->getInfo(cinfo[t]);
				delete rle;
			}
		}
		genfunc_next(gen, cinfo[0]);
		genfunc_next(gen, cinfo[1]);
		//parsing doubles
		int numreplaced;

		//function for quantity
		ColumnInfo quantity_info;
		quantity_info.colname = "quantity";
		quantity_info.updateAggregation = true;
		Function *quantity_fn = genfunc_readDouble(gen,quantity_info);
		gen->ReplaceCallSites(quantity_fn,true,genfunc_hash(gen),"hash",&numreplaced);
		gen->ReplaceCallSites(quantity_fn,true,genfunc_process_quantity(gen),"process",&numreplaced);

		//function for price
		ColumnInfo price_info;
		price_info.colname = "price";
		price_info.saveResult = true;
		price_info.updateAggregation = true;
		Function *price_fn = genfunc_readDouble(gen,price_info);
		gen->ReplaceCallSites(price_fn,true,genfunc_hash(gen),"hash",&numreplaced);
		gen->ReplaceCallSites(price_fn,true,genfunc_process_price(gen),"process",&numreplaced);

		//function for discount
		ColumnInfo discount_info;
		discount_info.colname = "discount";
		discount_info.updateAggregation = true;
		Function *discount_fn = genfunc_readDouble(gen, discount_info);
		gen->ReplaceCallSites(discount_fn,true,genfunc_hash(gen),"hash",&numreplaced);
		gen->ReplaceCallSites(discount_fn,true,genfunc_process_discount(gen),"process",&numreplaced);

		//function for tax
		ColumnInfo tax_info;
		tax_info.colname = "tax";
		tax_info.updateAggregation = true;
		Function *tax_fn = genfunc_readDouble(gen, tax_info);
		gen->ReplaceCallSites(tax_fn,true,genfunc_hash(gen),"hash",&numreplaced);
		gen->ReplaceCallSites(tax_fn,true,genfunc_process_tax(gen),"process",&numreplaced);

		//gen->EnableOptimizations(false);
		gen->FinalizeModule();

	}



	//generate function for RLE
	uint64_t funcaddr_return = gen->GetFunction("nextFunc_l_returnflag");
	uint64_t funcaddr_status = gen->GetFunction("nextFunc_l_linestatus");
	int64_t (*next[2])(unsigned char *, int64_t *, uint64_t &, uint64_t);
	next[0] = (int64_t (*)(unsigned char *, int64_t*, uint64_t &, uint64_t))funcaddr_return;
	next[1] = (int64_t (*)(unsigned char *, int64_t*, uint64_t &, uint64_t))funcaddr_status;

	//generate function for doubles
	uint64_t funcaddr_quantity = gen->GetFunction("nextFunc_quantity");
	uint64_t funcaddr_price = gen->GetFunction("nextFunc_price");
	uint64_t funcaddr_discount = gen->GetFunction("nextFunc_discount");
	uint64_t funcaddr_tax = gen->GetFunction("nextFunc_tax");

	void (*next_quantity)(unsigned char* data, double *result,uint64_t &index, uint64_t offset,
			uint64_t length, char *bitmap, double ** IRresult, int64_t **GPresult, double **aggresult)
			= 	(void (*)(unsigned char*, double *,uint64_t &, uint64_t , uint64_t , char *, double ** , int64_t **, double **))funcaddr_quantity;
	void (*next_price)(unsigned char* data, double *result,uint64_t &index, uint64_t offset,
			uint64_t length, char *bitmap, double ** IRresult, int64_t **GPresult, double **aggresult)
			= 	(void (*)(unsigned char*, double *,uint64_t &, uint64_t , uint64_t , char *, double ** , int64_t **, double **))funcaddr_price;
	void (*next_discount)(unsigned char* data, double *result,uint64_t &index, uint64_t offset,
			uint64_t length, char *bitmap, double ** IRresult, int64_t **GPresult, double **aggresult)
				= 	(void (*)(unsigned char*, double *,uint64_t &, uint64_t , uint64_t , char *, double ** , int64_t **, double **))funcaddr_discount;
	void (*next_tax)(unsigned char* data, double *result,uint64_t &index, uint64_t offset,
			uint64_t length, char *bitmap, double ** IRresult, int64_t **GPresult, double **aggresult)
				= 	(void (*)(unsigned char*, double *,uint64_t &, uint64_t , uint64_t , char *, double ** , int64_t **, double **))funcaddr_tax;

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


	int round = 1024000;
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (folderpath.c_str())) != NULL) {
	  /* print all the files and directories within directory */
	  while ((ent = readdir (dir)) != NULL) {
		  if(ent->d_type==DT_REG){
			std::unique_ptr<orc::Reader> reader = orc::createReader(orc::readLocalFile(folderpath+ent->d_name),opts);
			for(int i=0;i<reader->getStripeSize();i++){

				proto::StripeInformation sinfo = reader->getStrips(i);
				uint64_t nrows = sinfo.numberofrows();
				char *bitmap = new char[nrows];
				int64_t *GPresult[2];
				GPresult[0] = new int64_t[nrows];
				GPresult[1] = new int64_t[nrows];

				double *IRresult[1];
				IRresult[0] = new double[nrows];

				//GPresult, returnflag, linestatus
				for(int t=0;t<2;t++){
					unsigned char *data;
					uint64_t datasize = 0;
					reader->readdata(i,col[t],proto::Stream_Kind_DATA, &data,&datasize);

					uint64_t index=0;
					uint64_t offset = 0;
					while(index<datasize){
						offset += next[t](data,GPresult[t],index,offset);
					}
					if(data){
						delete data;
					}
				}

				//next quantity
				{
					unsigned char *data;
					uint64_t datasize = 0;
					reader->readdata(i,colmap["l_quantity"],proto::Stream_Kind_DATA, &data,&datasize);

					uint64_t index=0;
					uint64_t offset = 0;
					for(;offset<nrows;offset+=round){
						next_quantity(data,0,index, offset,(nrows-offset)>round?round:(nrows-offset),0,0,GPresult,aggresult);
					}
					if(data){
						delete data;
					}
				}

				//next price
				{
					unsigned char *data;
					uint64_t datasize = 0;
					reader->readdata(i,colmap["l_extendedprice"],proto::Stream_Kind_DATA, &data,&datasize);

					uint64_t index=0;
					uint64_t offset = 0;
					for(;offset<nrows;offset+=round){
						next_price(data,IRresult[0],index, offset,(nrows-offset)>round?round:(nrows-offset),0,0,GPresult,aggresult);
					}
					if(data){
						delete data;
					}
				}

				//next discount
				{
					unsigned char *data;
					uint64_t datasize = 0;
					reader->readdata(i,colmap["l_discount"],proto::Stream_Kind_DATA, &data,&datasize);

					uint64_t index=0;
					uint64_t offset = 0;
					for(;offset<nrows;offset+=round){
						next_discount(data,0,index, offset,(nrows-offset)>round?round:(nrows-offset),0,IRresult,GPresult,aggresult);
					}
					if(data){
						delete data;
					}
				}

				//next tax
				{
					unsigned char *data;
					uint64_t datasize = 0;
					reader->readdata(i,colmap["l_tax"],proto::Stream_Kind_DATA, &data,&datasize);

					uint64_t index=0;
					uint64_t offset = 0;
					for(;offset<nrows;offset+=round){
						next_tax(data,0,index, offset,(nrows-offset)>round?round:(nrows-offset),0,IRresult,GPresult,aggresult);
					}
					if(data){
						delete data;
					}
				}

				delete GPresult[0];
				delete GPresult[1];
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
