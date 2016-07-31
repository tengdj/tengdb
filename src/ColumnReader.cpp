/*
 * ColumnReader.cpp
 *
 *  Created on: Jul 24, 2016
 *      Author: teng
 */
#include <memory>

namespace orc{


void readDouble(unsigned char *data, double *result, uint64_t &index, uint64_t offset, uint64_t length){

   uint64_t end = offset+length;
   for(uint64_t t=offset;t<end;t++){
	   	int64_t bits = 0;
		bits |= static_cast<int64_t>(data[index++]);
		bits |= static_cast<int64_t>(data[index++]) << 8;
		bits |= static_cast<int64_t>(data[index++]) << 16;
		bits |= static_cast<int64_t>(data[index++]) << 24;
		bits |= static_cast<int64_t>(data[index++]) << 32;
		bits |= static_cast<int64_t>(data[index++]) << 40;
		bits |= static_cast<int64_t>(data[index++]) << 48;
		bits |= static_cast<int64_t>(data[index++]) << 56;
	    double *dresult = reinterpret_cast<double*>(&bits);
	    result[t] = *dresult;
  }

}



}
