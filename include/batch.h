/*
 * batch.h
 *
 *  Created on: Nov 11, 2016
 *      Author: teng
 */

#ifndef INCLUDE_BATCH_H_
#define INCLUDE_BATCH_H_
#include "util.h"
#include <map>
#include <vector>
#include "util.h"
#include <memory>
#include "functions-ir.h"
#include <iostream>
#include "MemoryPool.h"
namespace orc{

struct Batch{

	uint64_t counter = 0;

	std::map<std::string, DataBuffer<unsigned char> *> data;
	std::vector<Column> columns;
	uint64_t rownumber;
	uint64_t capacity;

	bool eof;

	void print(){
		if(counter==0){
			cout<<"#\t";
			for(Column col:columns){
				cout<<col.name<<"\t";
			}
			cout<<endl;
		}

		void *dataarray[columns.size()];
		for(int i=0;i<columns.size();i++){
			dataarray[i] = (void *)data[columns[i].name]->data();
		}
		for(uint64_t i=0;i<rownumber;i++){
			cout<<++counter<<"\t";
			int8_t byte;
			for(int j=0;j<columns.size();j++){

				switch(columns[j].type){
				case DOUBLE:
					cout<<((double *)dataarray[j])[i]<<"\t";
					break;
				case FLOAT:
					cout<<((float *)dataarray[j])[i]<<"\t";
					break;
				case LONG:
					cout<<((int64_t *)dataarray[j])[i]<<"\t";
					break;
				case STRING:
				case INT:
				case VARCHAR:
					cout<<((int32_t *)dataarray[j])[i]<<"\t";
					break;
				case DATE:
				case SHORT:
					cout<<((int16_t *)dataarray[j])[i]<<"\t";
					break;
				case BYTE:
					byte = ((int8_t *)dataarray[j])[i];
					cout<<(int16_t)byte<<"\t";
					break;
				default:
					cout<<"unknown type"<<"\t";
				}
			}
			cout<<endl;
		}
	}

	void resize(uint64_t rownumber){

		if(rownumber>this->rownumber){
			for(Column col:columns){
				data[col.name]->resize(rownumber*8);
			}
		}
		this->rownumber = rownumber;
	}
	Batch(std::vector<Column> columns, uint64_t capacity){
		this->columns = columns;
		for(Column col:columns){
			data[col.name] = new DataBuffer<unsigned char>(*orc::getDefaultPool(),8*capacity);
		}
		this->capacity = capacity;
		this->rownumber = 0;
		eof = false;
	}

	~Batch(){
		for(Column col:columns){
			if(data[col.name]!=NULL){
				delete data[col.name];
			}

		}
	}


};

}

#endif /* INCLUDE_BATCH_H_ */
