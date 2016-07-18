/*
 * general-ir.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: teng
 */

#include "../../include/util.hh"
#include <stdint.h>


uint64_t readDirect_general(char *data, int64_t *result, uint64_t &index, uint64_t &resultindex){

	char firstbyte = data[index++];
	char secondbyte = data[index++];
	uint32_t  bitSize = orc::decodeBitWidth((unsigned char)((firstbyte >> 1) & 0x1f));
	uint64_t runLength = static_cast<uint64_t>(firstbyte & 0x01) << 8;
	runLength |= (unsigned char)secondbyte;
	runLength += 1;
	char curByte;
	uint32_t bitsLeft = 0;
	for(int tt=0;tt<runLength;tt++){
		uint64_t rawresult = 0;
		uint64_t bitsLeftToRead = bitSize;
		if(bitsLeft==0){
		    curByte = data[index++];
		    bitsLeft = 8;
		}
		while (bitsLeftToRead > bitsLeft) {
			rawresult <<= bitsLeft;
			rawresult |= curByte & ((1 << bitsLeft) - 1);
		    bitsLeftToRead -= bitsLeft;
		    curByte = data[index++];
		    bitsLeft = 8;
		}
		// handle the left over bits
		if (bitsLeftToRead > 0) {
			rawresult <<= bitsLeftToRead;
		    bitsLeft -= static_cast<uint32_t>(bitsLeftToRead);
		    rawresult |= (curByte >> bitsLeft) & ((1 << bitsLeftToRead) - 1);
		}
		result[resultindex++] = orc::unZigZag(rawresult);
		//cout<<result[resultindex-1]<<endl;
	}
	return runLength;
}
