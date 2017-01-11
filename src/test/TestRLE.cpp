/*
 * TestRLE.cpp
 *
 *  Created on: Nov 7, 2016
 *      Author: teng
 */

#include "../include/Reader.h"
#include "../include/RLE2.h"
#include <stdio.h>
#include <iostream>

using namespace orc;
using namespace std;

int main(int argc, char **argv){

	ReaderOptions opts;
	MemoryPool *pool = orc::getDefaultPool();
	DataBuffer<unsigned char> buf(*pool,100);
	std::string filepath = "/home/teng/orcfile/testpatched/000000_0";
	filepath = "/home/teng/orcfile/all/000000_0";
	Reader * reader = orc::createReader(orc::readLocalFile(filepath),opts);
	uint64_t datasize;
	reader->readdata(0,2,proto::Stream_Kind_DATA, buf,datasize);
	RLE2<int64_t,uint64_t> *rle_return = new RLE2<int64_t,uint64_t>(buf.data(), datasize, true);

	uint64_t nrows = reader->getStrips(0).numberofrows();
	//cout<<nrows<<endl;
	int64_t *result = new int64_t[nrows];
	ColumnInfo info("col1",INT);
	rle_return->next(result,info.rleinfo);
	for(int i=0;i<nrows;i++){
		cout<<result[i]<<endl;
	}

	delete reader;

	return 0;

}
