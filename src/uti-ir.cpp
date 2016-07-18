/*
 * uti-ir.cpp
 *
 *  Created on: Jul 13, 2016
 *      Author: teng
 */

#include <llvm/Pass.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MathExtras.h>
#include <algorithm>
#include <stdlib.h>
#include <memory>
#include <iostream>

#include "llvm-codegen.h"
#include "Reader.hh"
#include "util.hh"
#include "config.hh"
#include "codegen_function.hh"
#include "RLE2.hh"
#include "functions-ir.h"
#include "ColumnInfo.h"

using namespace llvm;

int  main(int argc, char **args){
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
