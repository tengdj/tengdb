/*
 * ColumnInfo.h
 *
 *  Created on: Jul 10, 2016
 *      Author: teng
 */

#ifndef SRC_COLUMNINFO_H_
#define SRC_COLUMNINFO_H_

#include "util.h"
#include <sstream>
#include <ostream>
#include <iostream>
#include "util.h"

using namespace std;
using namespace tengdb;

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
			data[col.name] = new DataBuffer<unsigned char>(*getDefaultPool(),8*capacity);
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
static uint64_t sortData(int listlength, int *data, int *toplist, float *percentage){
	int min_index = 0;
	int min_value = data[min_index];
	for(int i=0;i<listlength;i++){
		if(data[i]<min_value){
			min_value = data[i];
			min_index = i;
		}
	}
	for(int i=0;i<listlength;i++){
		toplist[i] = min_index;
		percentage[i] = 0;
	}
	int tmp;
	uint64_t total = 0;
	for(int i=0;i<listlength;i++){
		total += data[i];
		toplist[listlength-1] = i;
		for(int j=listlength-1;j>=1;j--){
			//cout<<toplist[j]<<" "<<toplist[j-1]<<endl;
			if(data[toplist[j]]>data[toplist[j-1]]){
				tmp = toplist[j];
				toplist[j] = toplist[j-1];
				toplist[j-1] = tmp;
			}else{
				break;
			}
		}
	}

	for(int i=0;i<listlength;i++){
		percentage[i] = (float)data[toplist[i]]*100/total;
	}
	return total;
}

struct Column{
	std::string name;
	TypeKind type;
	Column(){name = "";type = INT;};
	Column(std::string name,TypeKind type){
		this->name = name;
		this->type = type;
	}
	Column(const Column &column){
		name = column.name;
		type = column.type;
	}

	bool isRleType(){
		return type!=DOUBLE&&type!=FLOAT;
	}
};

struct EncodingInfo{
	bool finalized;
	int bitsize[65];
	int runLength[513];
	int top_bitSize[65];
	float top_bitSize_percent[65];
	int top_runLength[513];
	float top_runLength_percent[513];
	int maxbitsize;


	bool issigned;
	uint64_t num;
	uint64_t totalRunLength;


	EncodingInfo(){
		for(int i=0;i<65;i++){
			bitsize[i] = 0;
			top_bitSize_percent[i] = 0;
		}
		for(int i=0;i<513;i++){
			runLength[i] = 0;
			top_runLength_percent[i] = 0;
		}
		issigned = true;
		num = 0;
		finalized = false;
		totalRunLength = 0;
		maxbitsize = 0;
	}

	void finalize(){
		finalized = true;
		sortData(65,bitsize,top_bitSize,top_bitSize_percent);
		sortData(513,runLength,top_runLength,top_runLength_percent);
		for(int i=65;i>=0;i--){
			if(bitsize[i]!=0){
				maxbitsize = i;
				break;
			}
		}


	}

	std::string getTopBitSize(int top){
		assert(finalized);
		std::ostringstream os;
		os<<"bitsize:\t";
		for(int i=0;i<top;i++){
			if(top_bitSize_percent[i]>0){
				os<<top_bitSize[i]<<"("<<top_bitSize_percent[i]<<"%)\t";
			}
		}
		return os.str();

	}

	std::string getTopRunLength(int top){
		assert(finalized);
		std::ostringstream os;
		os<<"runlength:\t";
		for(int i=0;i<top;i++){
			if(top_runLength_percent[i]>0){
				os<<top_runLength[i]<<"("<<top_runLength_percent[i]<<"%)\t";
			}else{
				break;
			}
		}
		return os.str();

	}

	bool isBitSizeFixed(){
		assert(finalized);
		return bitsize[top_bitSize[1]]==0;
	}

	int getFixedBitSize(){
		assert(finalized);
		if(bitsize[top_bitSize[1]]==0){
			return top_bitSize[0];
		}else{
			return -1;
		}
	}

	int getMinRunlength(){
		assert(finalized);
		for(int i=0;i<513;i++){
			if(runLength[i]!=0){
				return i;
			}
		}
	}


	void merge(EncodingInfo *B){
		assert(!finalized);
		this->num += B->num;
		for(int i=0;i<65;i++){
			bitsize[i] += B->bitsize[i];
		}
		for(int i=0;i<513;i++){
			runLength[i] += B->runLength[i];
		}

	}

};

struct RLEInfo{

	EncodingInfo *directInfo;
	EncodingInfo *repeatInfo;
	EncodingInfo *patchedInfo;
	EncodingInfo *deltaInfo;

	bool hasDirect(){
		return directInfo&&directInfo->num>0;
	}

	bool hasRepeat(){
		return repeatInfo&&repeatInfo->num>0;
	}

	bool hasPatched(){
		return patchedInfo&&patchedInfo->num>0;
	}

	bool hasDelta(){
		return deltaInfo&&deltaInfo->num>0;
	}
	RLEInfo(){

		directInfo = new EncodingInfo();
		repeatInfo = new EncodingInfo();
		patchedInfo = new EncodingInfo();
		deltaInfo = new EncodingInfo();

	}

	~RLEInfo(){

		delete directInfo;
		delete repeatInfo;
		delete patchedInfo;
		delete deltaInfo;

	}

	void finalize(){
		directInfo->finalize();
		repeatInfo->finalize();
		deltaInfo->finalize();
		patchedInfo->finalize();

	}
	void merge(RLEInfo *B){
		this->directInfo->merge(B->directInfo);
		this->repeatInfo->merge(B->repeatInfo);
		this->deltaInfo->merge(B->deltaInfo);
		this->patchedInfo->merge(B->patchedInfo);
	}

	void unsign(){
		directInfo->issigned = false;
		repeatInfo->issigned = false;
		deltaInfo->issigned = false;
		patchedInfo->issigned = false;

	}

	std::string toString(){
		
		uint64_t totalRunLength = 0;
		totalRunLength += repeatInfo->totalRunLength;
		totalRunLength += directInfo->totalRunLength;
		totalRunLength += deltaInfo->totalRunLength;
		totalRunLength += patchedInfo->totalRunLength;
		uint64_t totalnumber = 0;
		totalnumber += repeatInfo->num;
		totalnumber += directInfo->num;
		totalnumber += deltaInfo->num;
		totalnumber += patchedInfo->num;




		std::ostringstream os;
		
		os<<"average run length:	"<<(float)totalRunLength/totalnumber<<"\n";
		if(repeatInfo->num!=0){
			os<<"repeat:	"<<repeatInfo->num<<"("<<(repeatInfo->totalRunLength*100)/totalRunLength<<"%)\n\t"<<repeatInfo->getTopBitSize(FLAGS_fetch_top)<<"\n\t"<<repeatInfo->getTopRunLength(FLAGS_fetch_top)<<"\n";
		}
		if(directInfo->num!=0){
			os<<"direct:	"<<directInfo->num<<"("<<(directInfo->totalRunLength*100)/totalRunLength<<"%)\n\t"<<directInfo->getTopBitSize(FLAGS_fetch_top)<<"\n\t"<<directInfo->getTopRunLength(FLAGS_fetch_top)<<"\n";
		}
		if(deltaInfo->num!=0){
			os<<"delta:	"<<deltaInfo->num<<"("<<(deltaInfo->totalRunLength*100)/totalRunLength<<"%)\n\t"<<deltaInfo->getTopBitSize(FLAGS_fetch_top)<<"\n\t"<<deltaInfo->getTopRunLength(FLAGS_fetch_top)<<"\n";
		}
		if(patchedInfo->num!=0){
			os<<"patch:	"<<patchedInfo->num<<"("<<(patchedInfo->totalRunLength*100)/totalRunLength<<"%)\n\t"<<patchedInfo->getTopBitSize(FLAGS_fetch_top)<<"\n\t"<<patchedInfo->getTopRunLength(FLAGS_fetch_top)<<"\n";
		}

		return os.str();
	}
};
struct ColumnInfo{

	Column column;
	RLEInfo *rleinfo;

	ColumnInfo(std::string colname, TypeKind kind){
		this->column.name = colname;
		this->column.type = kind;
		if(column.isRleType()){
			rleinfo = new RLEInfo();
			if(kind==STRING||kind==VARCHAR){
				rleinfo->unsign();
			}
		}else{
			rleinfo = NULL;
		}


	}

	~ColumnInfo(){
		if(rleinfo){
			delete rleinfo;
		}
	}

	void merge(ColumnInfo *B){
		if(rleinfo){
			this->rleinfo->merge(B->rleinfo);
		}
	}

	void finalize(){
		rleinfo->finalize();
	}

};


}


#endif /* SRC_COLUMNINFO_H_ */
