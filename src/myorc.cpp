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
#include "functions.h"

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

	proto::Stream_Kind kind;
	string filepath;
	bool issigned;
	int column = 1;
	string colname;

	//filepath = "/home/teng/orcfile/testdirect/000000_0";kind = proto::Stream_Kind_DATA;issigned=true;colname="direct";;
	//filepath = "/home/teng/orcfile/testrepeat/000000_0";kind = proto::Stream_Kind_DATA;issigned=true;colname="repeat";
	//filepath = "/home/teng/orcfile/testdelta/000000_0";kind = proto::Stream_Kind_DATA;issigned=true;colname="delta";
	//filepath = "/home/teng/orcfile/testdecimal/000000_0";kind = proto::Stream_Kind_SECONDARY;issigned=false;colname="decimal";
	//filepath = "/home/teng/orcfile/testdictionary/000000_0";kind = proto::Stream_Kind_DATA;issigned = false;colname="dictionary";
	filepath = "/home/teng/orcfile/all/000000_0";kind = proto::Stream_Kind_DATA;issigned = false;column = 10;colname = "l_partkey";


	//filepath = "/home/teng/orcfile/testmix/000000_0";kind = proto::Stream_Kind_LENGTH;
	ReaderOptions opts;
	std::unique_ptr<orc::Reader> reader = orc::createReader(orc::readLocalFile(filepath),opts);

	ObjectPool pool;
	LlvmCodeGen *gen;
	bool local = argc>1;
	if(local){
		orc::LlvmCodeGen::LoadFromFile(&pool,"/home/teng/orcir/testutil_opt.ll","testutil",&gen);
	}else{
		gen = new LlvmCodeGen(&pool,"testutil");
		ColumnInfo cinfo;
		cinfo.colname = colname;
		for(int i=0;i<reader->getStripeSize();i++){
			unsigned char *data;
			uint64_t datasize;
			reader->readdata(i,column,kind, &data,&datasize);
			RLE2 *rle = new RLE2(data, datasize, issigned);
			rle->getInfo(cinfo);
			delete rle;
		}

		cout<<cinfo.hasrepeat<<" "<<cinfo.repeatInfo.isbitwidthfixed<<" "<<cinfo.repeatInfo.bitwidth<<endl;
		cout<<cinfo.hasdirect<<" "<<cinfo.directInfo.isbitwidthfixed<<" "<<cinfo.directInfo.bitwidth<<endl;
		cout<<cinfo.hasdelta<<" "<<cinfo.deltaInfo.isbitwidthfixed<<" "<<cinfo.deltaInfo.bitwidth<<endl;
		genfunc_next(gen, cinfo);
		gen->EnableOptimizations(true);
		gen->FinalizeModule();
	}

	uint64_t funcaddr = gen->GetFunction("nextFunc_"+colname);
	int64_t (*next)(unsigned char *, int64_t *, uint64_t &, uint64_t) = (int64_t (*)(unsigned char *, int64_t*, uint64_t &, uint64_t))funcaddr;

	for(int i=0;i<reader->getStripeSize();i++){

		unsigned char *data;
		uint64_t datasize;

		proto::StripeInformation sinfo = reader->getStrips(i);
		uint64_t nrows = sinfo.numberofrows();
		int64_t *result = new int64_t[nrows];
		uint64_t index=0;
		uint64_t offset = 0;
		reader->readdata(i,column,kind, &data,&datasize);


		//cout<<static_cast<uint32_t>(data[0])<<" "<<static_cast<uint32_t>(data[1])<<std::endl;
		int count[2];
		count[0] = 0;
		count[1] = 0;

		while(index<datasize){
			uint64_t runlength;
			runlength = next(data,result,index,offset);
			for(int r=offset;r<offset+runlength;r++){
				cout<<result[r]<<endl;
			}
			offset += runlength;
			//cout<<index<<endl;
			//cout<<runlength<<" "<<index<<endl;


			//*result += runlength*sizeof(uint64_t);
//			if(runlength!=512)

		}
		//cout<<count[0]<<" "<<count[1]<<endl;

		delete data;
		delete result;
	}

	return 0;
}

int main(int argc, char **argv){
	ReaderOptions opts;

	string file = "/home/teng/orcfile/all/000000_0";
	proto::Stream_Kind kind = proto::Stream_Kind_DATA;
	//file = "/home/teng/lineitem_orc_comment/000000_0";kind = proto::Stream_Kind_LENGTH;

	std::unique_ptr<orc::Reader> reader = orc::createReader(orc::readLocalFile(file),opts);
	ColumnInfo info;
	ObjectPool pool;
	LlvmCodeGen *gen;
	bool local = argc>1;
	if(local){
		orc::LlvmCodeGen::LoadFromFile(&pool,"/home/teng/orcir/readdouble_opt.ll","readdouble",&gen);
	}else{
		gen = new LlvmCodeGen(&pool,"readdouble");
		ColumnInfo cinfo;
		cinfo.colname = "tax";
		cinfo.saveResult = true;
		genfunc_readDouble(gen, cinfo);
		gen->EnableOptimizations(true);

		gen->FinalizeModule();
	}

	uint64_t funcaddr = gen->GetFunction("nextFunc_tax");
	void (*next)(unsigned char* data, double *result,uint64_t &index, uint64_t offset, uint64_t length, char *bitmap, double ** IRresult, uint64_t **GPresult, double **aggresult)
			= 	(void (*)(unsigned char*, double *,uint64_t &, uint64_t , uint64_t , char *, double ** , uint64_t **, double **))funcaddr;

	for(int i=0;i<reader->getStripeSize();i++){
		unsigned char *data;
		uint64_t datasize;

		proto::StripeInformation sinfo = reader->getStrips(i);
		uint64_t nrows = sinfo.numberofrows();
		int64_t *length = new int64_t[nrows];

		reader->readdata(i,7,kind, &data,&datasize);
		double result[512];
		uint64_t index = 0;
		uint64_t offset = 0;
		for(;offset<nrows;offset+=512){
			next(data,result,index, 0,(nrows-offset)>512?512:(nrows-offset),0,0,0,0);
			for(int t = 0;t<10;t++){
				cout<<"hhe: "<<result[t]<<endl;
			}
//			exit(0);
		}
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

	uint64_t (*process)(unsigned char *, int64_t *, uint64_t &, uint64_t &) = (uint64_t (*)(unsigned char *, int64_t *, uint64_t &, uint64_t &))funcaddr;

	ReaderOptions opts;
	std::unique_ptr<orc::Reader> reader = orc::createReader(orc::readLocalFile("/home/teng/testrepeat/000000_0"),opts);

	proto::Stream_Kind kind = proto::Stream_Kind_DATA;
	for(int i=0;i<reader->getStripeSize();i++){
		unsigned char *data;
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

