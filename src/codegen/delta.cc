/*
 * delta.cc
 *
 *  Created on: Jul 12, 2016
 *      Author: teng
 */

uint64_t readVslong(char *data, uint64_t &index);
uint64_t readLongs(char *data, int64_t *result, uint64_t &index, uint64_t offset, uint64_t len, uint64_t fb);

uint64_t nextDelta(char *data, int64_t *result, uint64_t &index, uint64_t offset){

	char firstByte = data[index++];
	// extract the number of fixed bits
	unsigned char fbo = (firstByte >> 1) & 0x1f;
	uint64_t bitSize = 0;
	uint64_t runLength;
	if (fbo != 0) {
	   bitSize = decodeBitWidth(fbo);
	}

	// extract the run length
	runLength = static_cast<uint64_t>(firstByte & 0x01) << 8;
	runLength |= data[index++];
	runLength += 1; // account for first value

	// read the first value stored as vint

	int64_t firstValue;
	firstValue = static_cast<int64_t>(readVslong(data,index));


	// read the fixed delta value stored as vint (deltas can be negative even
	// if all number are positive)
	int64_t deltaBase = static_cast<int64_t>(readVslong(data,index));

	uint64_t pos = offset;
	uint64_t end = offset + runLength;
	int64_t prevValue = firstValue;

	result[pos++] = firstValue;

	if (bitSize == 0) {
	    // add fixed deltas to adjacent values
	    for ( ;pos < end;) {
	      prevValue = result[pos++] = prevValue + deltaBase;
	    }
	} else {
	    if (pos < end) {
	      // add delta base and first value
	      prevValue = result[pos++] = firstValue + deltaBase;
	    }

	    // write the unpacked values, add it to previous value and store final
	    // value to result buffer. if the delta base value is negative then it
	    // is a decreasing sequence else an increasing sequence
	    uint64_t remaining = end - pos;
	    readLongs(data, result, index, pos, remaining, bitSize);

	    if (deltaBase < 0) {
	      for ( ; pos < end; ++pos) {
	        prevValue = result[pos] = prevValue - result[pos];
	      }
	    } else {
	      for ( ; pos < end; ++pos) {
	        prevValue = result[pos] = prevValue + result[pos];
	      }
	    }
	  }
	  return runLength;
}
