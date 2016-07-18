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
#include "Reader.hh"
#include "util.hh"
#include "config.hh"
#include "processColumn.hh"
#include "llvm-codegen.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "codegen_function.hh"
#include "RLE2.hh"
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

int main1(int argc, char **argv) {

	if(argc>1)
		orc::type = atoi(argv[1]);
	ReaderOptions opts;
	std::unique_ptr<orc::Reader> reader = orc::createReader(orc::readLocalFile("/home/teng/lineitem_orc/000000_0"),opts);
	char *data;
	uint64_t datasize;
	//reader->readdata(2,&data,&datasize);

	std::vector<ColumnPattern> patterns = readPattern(data,datasize);

	switch(type){
	case 1:
		processAddr = orc::genfunc();
		break;
	case 2:
	    processAddr = (uint64_t)orc::readDirect_general;
	    break;
	case 3:
	    processAddr = (uint64_t)orc::readDirect_spec;
		break;
	}
	void (*process)(char *, int64_t *, uint64_t &, uint64_t) = (void (*)(char *, int64_t *, uint64_t &, uint64_t))processAddr;

	uint64_t index = 0;
	uint64_t parsed = 0;

	int64_t result[512];

	for(ColumnPattern pattern:patterns){
		for(int i=0;i<pattern.repeated;i++){
			process(data,result,index,pattern.runLength);
			parsed += pattern.runLength;
		}
	}

	cout<<parsed<<""<<endl;
	if(data)
		delete data;
	return 0;
}

int main(int argc, char **argv){
	ReaderOptions opts;
	std::unique_ptr<orc::Reader> reader = orc::createReader(orc::readLocalFile("/home/teng/lineitem_orc_comment/000000_0"),opts);

	proto::Stream_Kind kind = proto::Stream_Kind_LENGTH;
	for(int i=0;i<reader->getStripeSize();i++){
		char *data;
		uint64_t datasize;

		proto::StripeInformation sinfo = reader->getStrips(i);
		uint64_t nrows = sinfo.numberofrows();
		int64_t *length = new int64_t[nrows];

		reader->readdata(i,1,kind, &data,&datasize);
		//cout<<datasize<<" "<<string(data,4)<<endl;

		RLE2 *rle = new RLE2(data, datasize, kind==proto::Stream_Kind_DATA);

		rle->read(length);

		for(int t=0;t<nrows;t++){
			cout<<length[t]<<endl;
		}
//
//		char *strdata;
//		uint64_t strdatasize;
//		reader->readdata(i,1,proto::Stream_Kind_DATA, &strdata, &strdatasize);
//		char *tmp = strdata;
//		delete strdata;


		delete rle;
		delete length;

	}

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

