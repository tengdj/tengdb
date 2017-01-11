/*
 * ORCScanner.cpp
 *
 *  Created on: Nov 11, 2016
 *      Author: teng
 */

#include <util.h>
#include <unistd.h>
#include <iostream>


#include "Scanner.h"
#include "functions-ir.h"
#include "RLE2.h"
using namespace std;
using namespace tengdb;
namespace orc{

ORCScanner::ORCScanner(Table *table,std::vector<Column> columns){
	this->table = table;
	this->curStripe = 0;
	this->reader = NULL;
	this->databuf = new DataBuffer<unsigned char>(*getDefaultPool(),100);
	for(Column col: columns){
		this->columns.push_back(col);
	}
	this->batch = new Batch(columns,1000000);
	table->reg();
}

void ORCScanner::prepare(){

	gen = this->table->getCodeGen();
	if(gen){
		this->gen = gen;
		for(Column col:columns){
			if(col.isRleType()&&!gen->hasFunction("next_"+col.name)){
				ColumnInfo *info = new ColumnInfo(col.name,col.type);
				this->colinfos[col.name] = info;
			}
		}
	}
}



Batch * ORCScanner::nextBatch(){

	if(reader==NULL||curStripe==reader->getStripeSize()){
		if(reader!=NULL){
			delete reader;
		}
		reader = table->nextFile();
		if(reader==NULL){
			batch->eof = true;
			return batch;
		}
		curStripe = 0;
	}
	batch->eof = false;

	uint64_t nrows = reader->getNumofRows(curStripe);
	batch->resize(nrows);
	uint64_t datasize = 0;

	for(Column col:columns){
		TypeKind kind = col.type;

		if(!col.isRleType()){
			reader->readdata(curStripe,table->getColumnNumber(col.name),Stream_Kind_DATA, *batch->data[col.name],datasize);
		}else{

			uint64_t index=0;
			uint64_t offset = 0;

			if(!gen||!gen->hasFunction("next_"+col.name)){
				switch(kind){
				case BYTE:
				{
					reader->readdata(curStripe,table->getColumnNumber(col.name),Stream_Kind_DATA, *databuf,datasize);
					unsigned char *data = databuf->data();
					void *result = (void *)batch->data[col.name]->data();
					RLE2<int8_t,uint8_t> *rle = new RLE2<int8_t,uint8_t>(data,datasize,true);
					rle->next((int8_t *)result,colinfos[col.name]->rleinfo);
					delete rle;
					break;
				}
				case SHORT:
				{
					reader->readdata(curStripe,table->getColumnNumber(col.name),Stream_Kind_DATA, *databuf,datasize);
					unsigned char *data = databuf->data();
					void *result = (void *)batch->data[col.name]->data();
					RLE2<int16_t,uint16_t> *rle = new RLE2<int16_t,uint16_t>(data,datasize,true);
					rle->next((int16_t *)result,colinfos[col.name]->rleinfo);
					delete rle;
					break;
				}
				case INT:
				{
					reader->readdata(curStripe,table->getColumnNumber(col.name),Stream_Kind_DATA, *databuf,datasize);
					unsigned char *data = databuf->data();
					void *result = (void *)batch->data[col.name]->data();
					RLE2<int32_t,uint32_t> *rle = new RLE2<int32_t,uint32_t>(data,datasize,true);
					rle->next((int32_t *)result,colinfos[col.name]->rleinfo);
					delete rle;
					break;
				}
				case LONG:
				{
					reader->readdata(curStripe,table->getColumnNumber(col.name),Stream_Kind_DATA, *databuf,datasize);
					unsigned char *data = databuf->data();
					void *result = (void *)batch->data[col.name]->data();
					RLE2<int64_t,uint64_t> *rle = new RLE2<int64_t,uint64_t>(data,datasize,true);
					rle->next((int64_t *)result,colinfos[col.name]->rleinfo);
					delete rle;
					break;
				}
				case DATE:
				{
					reader->readdata(curStripe,table->getColumnNumber(col.name),Stream_Kind_DATA, *databuf,datasize);
					unsigned char *data = databuf->data();
					void *result = (void *)batch->data[col.name]->data();
					RLE2<int16_t,uint16_t> *rle = new RLE2<int16_t,uint16_t>(data,datasize,true);
					rle->next((int16_t *)result,colinfos[col.name]->rleinfo);
					delete rle;
					break;
				}
				case STRING:
				{

					reader->readdata(curStripe,table->getColumnNumber(col.name),Stream_Kind_DATA, *databuf,datasize);
					unsigned char *data = databuf->data();
					void *result = (void *)batch->data[col.name]->data();
					RLE2<int32_t,uint32_t> *rle = new RLE2<int32_t,uint32_t>(data,datasize,false);
					rle->next((int32_t *)result,colinfos[col.name]->rleinfo);
					delete rle;
					break;
				}
				case VARCHAR:
				{
					reader->readdata(curStripe,table->getColumnNumber(col.name),Stream_Kind_LENGTH, *databuf,datasize);
					unsigned char *data = databuf->data();
					void *result = (void *)batch->data[col.name]->data();
					RLE2<int32_t,uint32_t> *rle = new RLE2<int32_t,uint32_t>(data,datasize,false);
					rle->next((int32_t *)result,colinfos[col.name]->rleinfo);
					delete rle;
					break;
				}
				default:
				{
					printf("wrong type!\n");
					exit(0);
				}


				}

			}else{
				if(kind==VARCHAR){
					reader->readdata(curStripe,table->getColumnNumber(col.name),Stream_Kind_LENGTH, *databuf,datasize);
				}else{
					reader->readdata(curStripe,table->getColumnNumber(col.name),Stream_Kind_DATA, *databuf,datasize);
				}
				unsigned char *data = databuf->data();
				void *result = (void *)batch->data[col.name]->data();
				next_func next = (next_func)gen->GetFunction("next_"+col.name);

				while(index<datasize){
					offset += next(data,index,offset,0,result);
					//cout<<index<<endl;
				}
			}
		}

	}

	curStripe++;
	return batch;

}



ORCScanner::~ORCScanner(){

	ORCScanner::mtx.lock();
	if(ORCScanner::colinfos_global.size()==0){
		for(Column col:columns){
			if(col.isRleType()&&gen&&!gen->hasFunction("next_"+col.name)){
				ColumnInfo *local = this->colinfos[col.name];
				ColumnInfo *global = new ColumnInfo(*local);
				ORCScanner::colinfos_global[col.name] = global;
			}
		}
	}else{
		for(Column col:columns){
			if(col.isRleType()&&gen&&!gen->hasFunction("next_"+col.name)){
				ColumnInfo *local = this->colinfos[col.name];
				ColumnInfo *global = ORCScanner::colinfos_global[col.name];
				global->merge(local);
			}
		}
	}
	ORCScanner::mtx.unlock();

	if(table->unreg()==0){
		if(FLAGS_codegen){
			bool genchanged = false;
			for(Column col:columns){
				if(col.isRleType()&&!gen->hasFunction("next_"+col.name)){
					ColumnInfo *colinfo = ORCScanner::colinfos_global[col.name];
					colinfo->finalize();
					cout<<col.name<<endl;
					cout<<colinfo->rleinfo->toString()<<endl;
					genfunc_next(gen,colinfo);
					delete colinfo;
					genchanged = true;
				}
			}
			if(genchanged){
				//gen->EnableOptimizations(false);
				//gen->setOptimizationLevel(0);
				gen->FinalizeModule();
			}
		}
		ORCScanner::colinfos_global.clear();
	}
	if(databuf){
		delete databuf;
	}
	if(reader){
		delete reader;
	}
	for(Column col:columns){
		ColumnInfo *info = this->colinfos[col.name];
		if(!info){
			delete info;
		}
	}
}

std::map<std::string,ColumnInfo *> ORCScanner::colinfos_global;
std::mutex ORCScanner::mtx;


}
