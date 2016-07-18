//============================================================================
// Name        : myorc.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdlib.h>
#include <memory>
#include "llvm-codegen.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "../include/config.h"
#include "../include/Reader.h"
#include "../include/RLE2.h"
#include "../include/util.h"
#include "functions-ir.h"

using namespace std;
using namespace orc;
using namespace llvm;

namespace orc{


uint64_t genfunc(){

	ObjectPool pool;
	LlvmCodeGen* gen;
	orc::LlvmCodeGen::LoadFromFile(&pool,"/home/teng/orcir/specific_opt.ll","specific",&gen);

//	LlvmCodeGen* gen = new LlvmCodeGen(&pool, "specific");
//	gen->EnableOptimizations(true);
//	Function *genfunc = genfunc_specific(gen);
//	gen->AddFunctionToJit(genfunc,(void**)&genfunc);
//	gen->FinalizeModule();

	return gen->GetFunction("readDirect_codegen");

}
}


int  main2(int argc, char **args){
	ObjectPool pool;
	LlvmCodeGen *gen;
	bool local = true;
	if(local){
		orc::LlvmCodeGen::LoadFromFile(&pool,"/home/teng/orcir/testutil_opt.ll","testutil",&gen);
	}else{
		gen = new LlvmCodeGen(&pool,"testutil");

		ColumnInfo cinfo;
		cinfo.hasdelta = true;
		cinfo.hasdirect = true;
		cinfo.haspatched = false;
		cinfo.hasrepeat = true;

		EncodingInfo info;
		info.issigned = false;
		info.isbitwidthfixed = false;
		info.bitwidth = 24;

		cinfo.directInfo = info;
		cinfo.repeatInfo = info;
		cinfo.deltaInfo = info;
		genfunc_next(gen, cinfo);
		gen->EnableOptimizations(true);
		gen->FinalizeModule();
	}



	uint64_t funcaddr = gen->GetFunction("nextFunc");
	int64_t (*next)(char *, int64_t *, uint64_t &, uint64_t) = (int64_t (*)(char *, int64_t*, uint64_t &, uint64_t))funcaddr;

	proto::Stream_Kind kind = proto::Stream_Kind_DATA;

	string filepath = "/home/teng/lineitem_orc_partkey/000000_0";
	filepath = "/home/teng/testrepeat/000000_0";
	filepath = "/home/teng/testdelta/000000_0";
	filepath = "/home/teng/lineitem_orc_comment/000000_0";kind = proto::Stream_Kind_LENGTH;
	ReaderOptions opts;
	std::unique_ptr<orc::Reader> reader = orc::createReader(orc::readLocalFile(filepath),opts);

	for(int i=0;i<reader->getStripeSize();i++){
		char *data;
		uint64_t datasize;

		proto::StripeInformation sinfo = reader->getStrips(i);
		uint64_t nrows = sinfo.numberofrows();
		int64_t *result = new int64_t[512];
		uint64_t resultindex = 0, index=0;
		reader->readdata(i,1,kind, &data,&datasize);


		//cout<<static_cast<uint32_t>(data[0])<<" "<<static_cast<uint32_t>(data[1])<<std::endl;
		int count[2];
		count[0] = 0;
		count[1] = 0;

		while(index<datasize){
			resultindex = 0;
			uint64_t offset = 0;
			uint64_t runlength;
			runlength = next(data,result,index,resultindex);

//			if(runlength!=512)
			//cout<<runlength<<" "<<index<<endl;
//			for(int r=0;r<runlength;r++){
//				cout<<result[r]<<endl;
//			}
		}
		//cout<<count[0]<<" "<<count[1]<<endl;

		delete data;
		delete result;

		//cout<<datasize<<" "<<string(data,4)<<endl;
	}

	return 0;
}

int main(int argc, char **argv){
	ReaderOptions opts;

	string file = "/home/teng/lineitem_orc_partkey/000000_0";
	proto::Stream_Kind kind = proto::Stream_Kind_DATA;
	file = "/home/teng/lineitem_orc_comment/000000_0";kind = proto::Stream_Kind_LENGTH;

	std::unique_ptr<orc::Reader> reader = orc::createReader(orc::readLocalFile(file),opts);
	ColumnInfo info;
	for(int i=0;i<reader->getStripeSize();i++){
		char *data;
		uint64_t datasize;

		proto::StripeInformation sinfo = reader->getStrips(i);
		uint64_t nrows = sinfo.numberofrows();
		int64_t *length = new int64_t[nrows];

		reader->readdata(i,1,kind, &data,&datasize);
		//cout<<datasize<<" "<<string(data,4)<<endl;

		RLE2 *rle = new RLE2(data, datasize, kind==proto::Stream_Kind_DATA);
		rle->getInfo(info);
		//rle->read(length);

//		rle->read(length);
//
//		for(int t=0;t<nrows;t++){
//			cout<<length[t]<<endl;
//		}
//
//		char *strdata;
//		uint64_t strdatasize;
//		reader->readdata(i,1,proto::Stream_Kind_DATA, &strdata, &strdatasize);
//		char *tmp = strdata;
//		delete strdata;


		delete rle;
		delete length;

	}
	cout<<info.hasrepeat<<" "<<info.repeatInfo.issigned<<" "<<info.repeatInfo.isbitwidthfixed<<" "<<info.repeatInfo.bitwidth<<endl;
	cout<<info.hasdirect<<" "<<info.directInfo.issigned<<" "<<info.directInfo.isbitwidthfixed<<" "<<info.directInfo.bitwidth<<endl;
	cout<<info.hasdelta<<" "<<info.deltaInfo.issigned<<" "<<info.deltaInfo.isbitwidthfixed<<" "<<info.deltaInfo.bitwidth<<endl;


	return 0;
}

int main3(int argc,char **argv){

	ObjectPool pool;
	LlvmCodeGen* gen;

	if(argc<=1){
		orc::LlvmCodeGen::LoadFromFile(&pool,"/home/teng/orcir/specific_opt.ll","specific",&gen);
	}else{
		EncodingInfo info;
		info.isbitwidthfixed = false;
		info.bitwidth = 24;
		info.issigned = true;
		info.isrunlengthfixed = false;
		gen = new LlvmCodeGen(&pool, "specific");
		gen->EnableOptimizations(true);
		Function *func_direct = genfunc_nextDirect(gen,info);
		gen->AddFunctionToJit(func_direct,(void**)&func_direct);
		Function *func_repeat = genfunc_nextRepeat(gen,info);
		gen->AddFunctionToJit(func_repeat,(void**)&func_repeat);
		gen->FinalizeModule();
	}

	return 0;
	uint64_t funcaddr = gen->GetFunction("nextShortRepeat");

	uint64_t (*process)(char *, int64_t *, uint64_t &, uint64_t &) = (uint64_t (*)(char *, int64_t *, uint64_t &, uint64_t &))funcaddr;

	ReaderOptions opts;
	std::unique_ptr<orc::Reader> reader = orc::createReader(orc::readLocalFile("/home/teng/testrepeat/000000_0"),opts);

	proto::Stream_Kind kind = proto::Stream_Kind_DATA;
	for(int i=0;i<reader->getStripeSize();i++){
		char *data;
		uint64_t datasize;

		proto::StripeInformation sinfo = reader->getStrips(i);
		uint64_t nrows = sinfo.numberofrows();
		int64_t *result = new int64_t[512];
		uint64_t resultindex = 0, index=0;
		reader->readdata(i,1,kind, &data,&datasize);

		while(index<datasize){
			resultindex = 0;
			uint64_t runlength = process(data,result,index,resultindex);
//			if(runlength!=512)
//			cout<<runlength<<endl;
			for(int r=0;r<resultindex;r++){
				//cout<<result[r]<<endl;
			}
		}
		//cout<<resultindex<<endl;

		delete data;
		delete result;

		//cout<<datasize<<" "<<string(data,4)<<endl;
	}
	return 0;
}

