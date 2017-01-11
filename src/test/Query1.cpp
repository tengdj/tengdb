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
#include <thread>
#include <mutex>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "../include/config.h"
#include "../include/Reader.h"
#include "../include/RLE2.h"
#include "../include/util.h"
#include "../include/MemoryPool.h"


#include "functions-ir.h"
#include "llvm-codegen.h"
#include "ProcessRow-ir.h"
#include "Table.h"

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
Table *lineitem;
double *aggresult[6];

void init(){

	lineitem = orc::getTable("lineitem");
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
}

//void init1(){
//	colmap["l_quantity"] = 1;
//	colmap["l_extendedprice"] = 2;
//	colmap["l_discount"] = 3;
//	colmap["l_tax"] = 4;
//	colmap["l_returnflag"] = 5;
//	colmap["l_linestatus"] = 6;
//	colmap["l_shipdate"] = 7;
//}

std::mutex mtx;

//generate function for RLE
int64_t (*next_return)(unsigned char *, uint64_t &, uint64_t, MemSpace &space, int64_t *);
int64_t (*next_status)(unsigned char *, uint64_t &, uint64_t, MemSpace &space, int64_t *);
//generate function for doubles
void (*next_quantity)(unsigned char* data, uint64_t &index, uint64_t offset,uint64_t length, MemSpace &);
void (*next_price)(unsigned char* data, uint64_t &index, uint64_t offset,uint64_t length, MemSpace &);
void (*next_discount)(unsigned char* data, uint64_t &index, uint64_t offset,uint64_t length, MemSpace &);
void (*next_tax)(unsigned char* data, uint64_t &index, uint64_t offset,uint64_t length, MemSpace &);
int total = 0;
void call_thread(){
	//0: sum quantity
	//1: sum price
	//2: sum discount
	//3: sum price*(1-discount)
	//4: sum price*(1-discount)*(1+tax)
	double *aggresult_tmp[6];
	aggresult_tmp[0] = new double[5];
	aggresult_tmp[1] = new double[5];
	aggresult_tmp[2] = new double[5];
	aggresult_tmp[3] = new double[5];
	aggresult_tmp[4] = new double[5];
	aggresult_tmp[5] = new double[5];

	for(int i=0;i<6;i++){
		for(int j=0;j<5;j++){
			aggresult_tmp[i][j] = 0;
		}
	}

	MemoryPool *mempool = getDefaultPool();
	DataBuffer<unsigned char> buf(*mempool,100);

	DataBuffer<char> bitmap_buf(*mempool,100);
	DataBuffer<int64_t> return_buf(*mempool,100);
	DataBuffer<int64_t> status_buf(*mempool,100);
	DataBuffer<uint64_t> hashresult_buf(*mempool,100);
	DataBuffer<double> iresult_buf(*mempool,100);

	int round = 102400;
	struct dirent *ent;
	uint64_t sum;
	MemSpace space;
	//space.bitMap = bitmap_buf.data();
	double *IRresult[1];
	space.doubleAggResult = aggresult_tmp;
	space.hashResult = hashresult_buf.data();
	space.doubleResult = IRresult;

	ReaderOptions opts;
	int totalrow = 0;
	  /* print all the files and directories within directory */
	while (true) {
		Reader *reader = lineitem->nextFile();
		if(ent==NULL){
			break;
		}

		for(int i=0;i<reader->getStripeSize();i++){

			proto::StripeInformation sinfo = reader->getStrips(i);
			uint64_t nrows = sinfo.numberofrows();
			totalrow += nrows;

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
				reader->readdata(i,lineitem->getColumnNumber("l_returnflag"),proto::Stream_Kind_DATA, buf,datasize);
				uint64_t index=0;
				uint64_t offset = 0;
				while(index<datasize){
					offset += next_return(buf.data(),index,offset,space,return_buf.data());
				}
			}
			//parse linestatus
			{
				uint64_t datasize = 0;
				reader->readdata(i,lineitem->getColumnNumber("l_linestatus"),proto::Stream_Kind_DATA, buf,datasize);

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
				reader->readdata(i,lineitem->getColumnNumber("l_quantity"),proto::Stream_Kind_DATA, buf,datasize);

				uint64_t index=0;
				uint64_t offset = 0;
				for(;offset<nrows;offset+=round){
					next_quantity(buf.data(),index, offset,(nrows-offset)>round?round:(nrows-offset),space);
				}
			}

			//next price
			{
				uint64_t datasize = 0;
				reader->readdata(i,lineitem->getColumnNumber("l_extendedprice"),proto::Stream_Kind_DATA, buf,datasize);

				uint64_t index=0;
				uint64_t offset = 0;
				for(;offset<nrows;offset+=round){
					next_price(buf.data(),index, offset,(nrows-offset)>round?round:(nrows-offset),space);
				}
			}

			//next discount
			{
				uint64_t datasize = 0;
				reader->readdata(i,lineitem->getColumnNumber("l_discount"),proto::Stream_Kind_DATA, buf,datasize);

				uint64_t index=0;
				uint64_t offset = 0;
				for(;offset<nrows;offset+=round){
					next_discount(buf.data(),index, offset,(nrows-offset)>round?round:(nrows-offset),space);
				}
			}

			//next tax
			{
				uint64_t datasize = 0;
				reader->readdata(i,lineitem->getColumnNumber("l_tax"),proto::Stream_Kind_DATA, buf,datasize);

				uint64_t index=0;
				uint64_t offset = 0;
				for(;offset<nrows;offset+=round){
					next_tax(buf.data(),index, offset,(nrows-offset)>round?round:(nrows-offset),space);
				}
			}


			//delete IRresult[0];

		}

		delete reader;
	}
	mtx.lock();
	for(int i=0;i<6;i++){
		for(int j=0;j<5;j++){
			aggresult[i][j] += aggresult_tmp[i][j];
		}
	}
	total += totalrow;
	mtx.unlock();
}

int  main(int argc, char **args){
	int threads_num = 1;
	bool load = false;
	for (int i = 1; i < argc; i++) {
	    double d;
	    int n;
	    int n2;
	    long long ll;
	    int64_t n64;
	    char junk;
	    if (sscanf(args[i], "-t=%d%c", &n, &junk) == 1) {
	    	threads_num = n;
	    } else if (sscanf(args[i], "-l=%d%c",&n, &junk) == 1) {
	      load = n;
	    } else {
	    	fprintf(stderr, "Invalid flag '%s'\n", args[i]);
	    	exit(1);
	    }
	  }
	init();

	string filepath = "/home/teng/orcfile/all/000000_0";
	ReaderOptions opts;

	ObjectPool pool;
	LlvmCodeGen *gen;
	MemoryPool *mempool = getDefaultPool();
	DataBuffer<unsigned char> buf(*mempool,100);

	if(load){
		orc::LlvmCodeGen::LoadFromFile(&pool,"./query1_opt.ll","query1",&gen);
	}else{

		Reader* reader = orc::createReader(orc::readLocalFile(filepath),opts);
		gen = new LlvmCodeGen(&pool,"query1");

		//gen func for group columns
		ColumnInfo *cinfo_return = new ColumnInfo("l_returnflag",INT);
		ColumnInfo *cinfo_status = new ColumnInfo("l_linestatus",INT);
		DataBuffer<int64_t> resultbuf(*mempool,100);
		for(int i=0;i<reader->getStripeSize();i++){
			uint64_t datasize;
			reader->readdata(i,lineitem->getColumnNumber("l_returnflag"),proto::Stream_Kind_DATA, buf,datasize);
			RLE2<int64_t,uint64_t> *rle_return = new RLE2<int64_t,uint64_t>(buf.data(), datasize, false);
			if(resultbuf.size()<reader->getStrips(i).numberofrows())
			resultbuf.resize(reader->getStrips(i).numberofrows());
			rle_return->next(resultbuf.data(),cinfo_return->rleinfo);
			delete rle_return;

			reader->readdata(i,lineitem->getColumnNumber("l_linestatus"),proto::Stream_Kind_DATA, buf,datasize);
			RLE2<int64_t,uint64_t> *rle_status = new RLE2<int64_t,uint64_t>(buf.data(), datasize, false);
			rle_status->next(resultbuf.data(),cinfo_status->rleinfo);
			delete rle_status;
		}

		genfunc_rle(gen, cinfo_return);
		genfunc_rle(gen, cinfo_status);

		delete reader;
		delete cinfo_return;
		delete cinfo_status;

		//gen func for select columns
		int numreplaced;
		ColumnInfo dinfo("",DOUBLE);
		Function *readDouble_func = genfunc_next(gen,&dinfo);

		Statement rowvalue_stmt(RETURNVALUE);
		//function for quantity
		ColumnOP quantity_op;
		quantity_op.op = OP::agg;
		quantity_op.dest = 0;
		quantity_op.statement = &rowvalue_stmt;
		std::vector<ColumnOP *> quantity_colops;
		quantity_colops.push_back(&quantity_op);
		Function *quantity_fn = gen->ReplaceCallSites(readDouble_func,false,genfunc_process(gen,quantity_colops),"process",&numreplaced);
		quantity_fn->setName("nextFunc_quantity");

		//function for price
		ColumnOP price_op[2];
		price_op[0].op = OP::store;
		price_op[0].dest = 0;
		price_op[0].statement = &rowvalue_stmt;
		price_op[1].op = OP::agg;
		price_op[1].dest = 1;
		price_op[1].statement = &rowvalue_stmt;
		std::vector<ColumnOP *> price_colops;
		price_colops.push_back(&price_op[0]);
		price_colops.push_back(&price_op[1]);
		Function *price_fn = gen->ReplaceCallSites(readDouble_func,false,genfunc_process(gen,price_colops),"process",&numreplaced);
		price_fn->setName("nextFunc_price");

		//function for discount
		Statement pricetTdiscount_stmt(HardCode::PRICETDISCOUNT);
		ColumnOP discount_op[3];
		discount_op[0].op = OP::agg;
		discount_op[0].dest = 2;
		discount_op[0].statement = &rowvalue_stmt;
		discount_op[1].op = OP::store;
		discount_op[1].dest = 0;
		discount_op[1].statement = &pricetTdiscount_stmt;
		discount_op[2].op = OP::agg;
		discount_op[2].dest = 3;
		discount_op[2].statement = &pricetTdiscount_stmt;
		std::vector<ColumnOP *> discount_colops;
		discount_colops.push_back(&discount_op[0]);
		discount_colops.push_back(&discount_op[1]);
		discount_colops.push_back(&discount_op[2]);
		Function *discount_fn = gen->ReplaceCallSites(readDouble_func,false,genfunc_process(gen,discount_colops),"process",&numreplaced);
		discount_fn->setName("nextFunc_discount");

		//function for tax
		Statement pricetTdiscountTtax_stmt(HardCode::PRICETDISCOUNTTTAX);
		ColumnOP tax_op;
		tax_op.op = agg;
		tax_op.dest = 4;
		tax_op.statement = &pricetTdiscountTtax_stmt;
		std::vector<ColumnOP *> tax_colops;
		tax_colops.push_back(&tax_op);
		Function *tax_fn = gen->ReplaceCallSites(readDouble_func,false,genfunc_process(gen,tax_colops),"process",&numreplaced);
		tax_fn->setName("nextFunc_tax");

		gen->EnableOptimizations(true);
		gen->setOptimizationLevel(3);
		gen->FinalizeModule();

	}



	//generate function for RLE
	next_return = (int64_t (*)(unsigned char *, uint64_t &, uint64_t, MemSpace &space, int64_t *iresult))gen->GetFunction("nextFunc_l_returnflag");
	next_status = (int64_t (*)(unsigned char *, uint64_t &, uint64_t, MemSpace &space, int64_t *iresult))gen->GetFunction("nextFunc_l_linestatus");;

	//generate function for doubles
	next_quantity	= 	(void (*)(unsigned char*, uint64_t &, uint64_t , uint64_t , MemSpace &))gen->GetFunction("nextFunc_quantity");
	next_price	= 	(void (*)(unsigned char*, uint64_t &, uint64_t , uint64_t , MemSpace &))gen->GetFunction("nextFunc_price");
	next_discount	= 	(void (*)(unsigned char*, uint64_t &, uint64_t , uint64_t , MemSpace &))gen->GetFunction("nextFunc_discount");
	next_tax	= 	(void (*)(unsigned char*, uint64_t &, uint64_t , uint64_t , MemSpace &))gen->GetFunction("nextFunc_tax");
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

	//int threads_num = sysconf(_SC_NPROCESSORS_ONLN)+2;
	std::thread t[threads_num];
	for(int i=0;i<threads_num;i++){
	    t[i] = std::thread(call_thread);
	}

	cout<<threads_num<<" threads are started"<<endl;
	for(int i=0;i<threads_num;i++){
	    t[i].join();
	}

	for(int i=0;i<6;i++){
		cout<<i;
		for(int j=0;j<5;j++){
			cout<<" "<<aggresult[i][j];
		}
		cout<<endl;
	}
	delete lineitem;
	printf("total:%d\n",total);
	return 0;
}

