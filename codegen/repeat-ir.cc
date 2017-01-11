/*
 * repeat-ir.cc
 *
 *  Created on: Jul 11, 2016
 *      Author: teng
 */
#include <stdint.h>

inline int64_t readLongBE(char *data, uint64_t &index, uint64_t bsz) {
  int64_t ret = 0, val;
  uint64_t n = bsz;
  while (n > 0) {
    n--;
    val = data[index++];
    ret |= (val << (n * 8));
  }
  return ret;
}



uint64_t nextShortRepeats(char *data, int64_t *result, uint64_t &index, uint64_t &resultindex){

	char firstByte = data[index++];
	uint64_t byteSize = (firstByte >> 3) & 0x07;
	byteSize += 1;

	uint64_t runLength = firstByte & 0x07;
	// run lengths values are stored only after MIN_REPEAT value is met
	runLength += 3;
	// read the repeated value which is store using fixed bytes
	int64_t value = readLongBE(data, index,byteSize);

	//value = unZigZag(static_cast<uint64_t>(value));

	uint64_t end = runLength + resultindex;
	//TODO modify for run time code generation
	for(; resultindex < end; ++resultindex) {
	      result[resultindex] = value;
	}
	return runLength;
}
