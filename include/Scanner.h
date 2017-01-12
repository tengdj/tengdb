/*
 * Scanner.h
 *
 *  Created on: Nov 11, 2016
 *      Author: teng
 */

#ifndef INCLUDE_SCANNER_H_
#define INCLUDE_SCANNER_H_

#include "Table.h"
#include <map>
#include <vector>
#include <mutex>
#include "ORCReader.h"
#include "llvm-codegen.h"
#include "ORCColumnInfo.h"

namespace tengdb{

class Scanner{
protected:
	Table *table;
	std::vector<Column> columns;
	Batch *batch;
	LlvmCodeGen *gen;


public:
	Scanner(){
		table = NULL;
		batch = NULL;
		gen = NULL;

	}
	Table *getTable(){
		return table;
	}
	Batch* nextBatch(){
		return batch;
	}
	~Scanner(){
		if(batch){
			delete batch;
		}
	};

	void prepare(LlvmCodeGen *gen){
		this->gen = gen;
	}
};

class ORCScanner:public Scanner{

protected:
	Reader* reader;
	uint64_t curStripe;
	DataBuffer<unsigned char> *databuf;
	std::map<std::string,ColumnInfo *> colinfos;

public:
	ORCScanner(Table *table,std::vector<Column> columns);
	void prepare();
	Batch* nextBatch();
	~ORCScanner();
	static std::map<std::string,ColumnInfo *> colinfos_global;
	static std::mutex mtx;


};

}


#endif /* INCLUDE_SCANNER_H_ */
