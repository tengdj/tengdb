/*
 * TestRLE-ir.cpp
 *
 *  Created on: Nov 8, 2016
 *      Author: teng
 */

#include "functions-ir.h"
#include "RLE2.h"
#include "ORCReader.h"

#include <iostream>

using namespace tengdb;
using namespace orc;


int  main(int argc, char **args){

	int threads_num = 1;
	bool ir = true;
	bool local = false;

	for (int i = 1; i < argc; i++) {
		double d;
		int n;
		int n2;
		long long ll;
		int64_t n64;
		char junk;
		if (sscanf(args[i], "-t=%d%c", &n, &junk) == 1) {
			threads_num = n;
		} else if (sscanf(args[i], "-i=%d%c",&n, &junk) == 1) {
		  ir = n;
		} else if (sscanf(args[i], "-l=%d%c",&n, &junk) == 1) {
		  local = n;
		} else {
		fprintf(stderr, "Invalid flag '%s'\n", args[i]);
		exit(1);
		}
	  }
	Stream_Kind kind;
	string filepath;
	bool issigned;
	int column = 1;
	string colname;

	//filepath = "/home/teng/orcfile/testdirect/000000_0";kind = proto::Stream_Kind_DATA;issigned=true;colname="direct";;
	//filepath = "/home/teng/orcfile/testrepeat/000000_0";kind = proto::Stream_Kind_DATA;issigned=true;colname="repeat";
	//filepath = "/home/teng/orcfile/testdelta/000000_0";kind = proto::Stream_Kind_DATA;issigned=true;colname="delta";
	//filepath = "/home/teng/orcfile/testdecimal/000000_0";kind = proto::Stream_Kind_SECONDARY;issigned=false;colname="decimal";
	//filepath = "/home/teng/orcfile/testdictionary/000000_0";kind = proto::Stream_Kind_DATA;issigned = false;colname="dictionary";
	filepath = "/home/teng/orcfile/allsmall/000000_0";kind = Stream_Kind_DATA;issigned = true;column = 1;colname = "l_partkey";
	//filepath = "/home/teng/orcfile/testpatched/000000_0";kind = proto::Stream_Kind_DATA;issigned=false;colname="patched";;


	//filepath = "/home/teng/orcfile/testmix/000000_0";kind = proto::Stream_Kind_LENGTH;
	ReaderOptions opts;
	Reader * reader = orc::createReader(orc::readLocalFile(filepath),opts);

	ObjectPool pool;
	LlvmCodeGen *gen;
	MemoryPool *mempool = getDefaultPool();
	DataBuffer<unsigned char> buf(*mempool,100);
	DataBuffer<int64_t> resultbuf(*mempool,100);

	if(local){
		LlvmCodeGen::LoadFromFile(&pool,"./testutil_opt.ll","testutil",&gen);
	}else{
		gen = new LlvmCodeGen(&pool,"testutil");
		ColumnInfo cinfo(colname,INT);
		for(int i=0;i<reader->getStripeSize();i++){
			uint64_t datasize;
			reader->readdata(i,column,kind, buf,datasize);
			RLE2<int64_t,uint64_t> *rle = new RLE2<int64_t,uint64_t>(buf.data(), datasize, issigned);
			uint64_t nrows = reader->getNumofRows(i);
			if(resultbuf.size()<nrows)
			resultbuf.resize(nrows);

			rle->next(resultbuf.data(),cinfo.rleinfo);
			if(!ir)
			for(int r=0;r<nrows;r++){
				cout<<r<<"	"<<resultbuf.data()[r]<<endl;
			}
			delete rle;
		}

		if(!ir){
			return 0;
		}


		cout<<cinfo.rleinfo->toString();

		genfunc_next(gen, &cinfo);
		gen->FinalizeModule();
		//gen->module()->dump();
	}

	uint64_t funcaddr = gen->GetFunction("next_"+colname);
	next_func next
			= (next_func)funcaddr;

	DataBuffer<int64_t> intbuf(*mempool,100);
	uint64_t total = 0;
	for(int i=0;i<reader->getStripeSize();i++){

		uint64_t datasize;

		uint64_t nrows = reader->getNumofRows(i);
		uint64_t index=0;
		uint64_t offset = 0;
		reader->readdata(i,column,kind, buf,datasize);

		if(intbuf.size()<nrows){
			intbuf.resize(nrows);
		}
		int64_t *result = intbuf.data();
		while(index<datasize)
		{
			uint64_t runlength;
			runlength = next(buf.data(),index,offset,0,(void *)result);
			for(int r=offset;r<offset+runlength;r++){
				cout<<total++<<"	"<<result[r]<<endl;

			}
//			cout<<"runlength:"<<runlength<<endl;
			offset += runlength;

		}

	}

	delete reader;

	return 0;
}
