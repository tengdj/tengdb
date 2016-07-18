/*
 * process-ir.cc
 *
 *  Created on: Jun 25, 2016
 *      Author: teng
 */

#include <stdint.h>

uint64_t readDirect_codegen(char *data, int64_t *result, uint64_t &index, uint64_t &resultindex){

	int repeated;
	uint64_t parsed;
	for(int i = 0;i<repeated;i++){
		index+=2;
		resultindex = 0;
		for(uint32_t tt=0;tt<512;tt++){
			uint32_t rawresult = 0;

			rawresult |= data[index++]&0xff;
			rawresult <<= 8;
			rawresult |= data[index++]&0xff;
			rawresult <<= 8;
			rawresult |= data[index++]&0xff;

			result[resultindex++] = rawresult >> 1 ^ -(rawresult & 1);;
			//cout<<result[resultindex-1]<<endl;
		}
	}
	return parsed;
}
