/*
 * process-ir.cc
 *
 *  Created on: Jun 25, 2016
 *      Author: teng
 */

#include <stdint.h>

uint64_t readDirect_codegen(char *data, int64_t *result, uint64_t &index, uint64_t &resultindex){

	char firstbyte = data[index++];
	char secondbyte = data[index++];
	//uint32_t  bitSize = 24;
	uint32_t runLength = static_cast<uint32_t>(firstbyte & 0x01) << 8;
	runLength |= (unsigned char)secondbyte;
	runLength += 1;
	for(uint32_t tt=0;tt<runLength;tt++){
		uint32_t rawresult = 0;

		rawresult |= data[index++]&0xff;
		rawresult <<= 8;
		rawresult |= data[index++]&0xff;
		rawresult <<= 8;
		rawresult |= data[index++]&0xff;

		result[resultindex++] = rawresult >> 1 ^ -(rawresult & 1);;
		//cout<<result[resultindex-1]<<endl;
	}
	return runLength;
}
