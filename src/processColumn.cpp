/*
 * processColumn.cpp
 *
 *  Created on: Jun 29, 2016
 *      Author: teng
 */


#include <vector>
#include <stdint.h>
#include <iostream>

#include "util.hh"
#include "processColumn.hh"
namespace orc{



ColumnPattern::ColumnPattern(uint64_t repeated, uint64_t bitSize, uint64_t runLength):
repeated(repeated),bitSize(bitSize),runLength(runLength){

}

ColumnPattern::ColumnPattern(const ColumnPattern &pattern):
repeated(pattern.repeated),bitSize(pattern.bitSize),runLength(pattern.runLength){

}

std::vector<ColumnPattern> readPattern(char *data, uint64_t datasize){

		std::vector<ColumnPattern> patterns;
		uint64_t index = 0;
		char firstbyte, secondbyte;
		uint32_t bitSize;
		uint64_t runLength;
		uint64_t total = 0;
		ColumnPattern pattern;
		do{

			char firstbyte = data[index++];
			char secondbyte = data[index++];
			bitSize = decodeBitWidth((unsigned char)((firstbyte >> 1) & 0x1f));
			runLength = static_cast<uint64_t>(firstbyte & 0x01) << 8;
			runLength |= (unsigned char)secondbyte;
			runLength += 1;
			total += runLength;
			index += (bitSize*runLength)/8;
			if(pattern.bitSize==bitSize&&pattern.runLength==runLength){
				pattern.repeated++;
			}else{
				if(pattern.repeated!=0){//not the first

					patterns.push_back(ColumnPattern(pattern));

				}
				pattern.bitSize = bitSize;
				pattern.runLength = runLength;
				pattern.repeated = 1;
			}
			if(index==datasize&&pattern.repeated!=0){
				patterns.push_back(ColumnPattern(pattern));
			}
			//std::cout<<bitSize<<" "<<runLength<<" "<<index<<std::endl;
		}while(index<datasize);
		return patterns;

	}

}
