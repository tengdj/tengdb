/*
 * RLE2.cpp
 *
 *  Created on: Jun 29, 2016
 *      Author: teng
 */

#include "../include/RLE2.h"

#include <stdint.h>
#include <iostream>

#include "../include/util.h"
#define MIN_REPEAT 3

namespace orc{

int64_t RLE2::readLongBE(uint64_t bsz) {
  int64_t ret = 0, val;
  uint64_t n = bsz;
  while (n > 0) {
    n--;
    val = data[index++];
    ret |= (val << (n * 8));
  }
  return ret;
}


uint64_t RLE2::readVulong() {
  uint64_t ret = 0, b;
  uint64_t offset = 0;
  do {
    b = data[index++];
    ret |= (0x7f & b) << offset;
    offset += 7;
  } while (b >= 0x80);
  return ret;
}

inline int64_t RLE2::readVslong() {
  return unZigZag(readVulong());
}


uint64_t RLE2::readLongs(int64_t *result, uint64_t offset, uint64_t len, uint64_t fb) {

  uint64_t bitsLeft = 0;
  unsigned char curByte;
  // TODO: unroll to improve performance
  for(uint64_t i = offset; i < (offset + len); i++) {

    uint64_t rawresult = 0;
    uint64_t bitsLeftToRead = fb;

//    rawresult |= data[index++] & 0xff;
//    rawresult <<= 8;
//    rawresult |= data[index++] & 0xff;
//

    if(bitsLeft==0){
    	curByte = data[index++];
    	bitsLeft = 8;
    }
    while (bitsLeftToRead > bitsLeft){

    	rawresult <<= bitsLeft;
    	rawresult |= curByte & ((1 << bitsLeft) - 1);
    	bitsLeftToRead -= bitsLeft;
    	curByte = data[index++];
    	bitsLeft = 8;
    }
    // handle the left over bits
    if (bitsLeftToRead > 0){

      rawresult <<= bitsLeftToRead;
      bitsLeft -= static_cast<uint32_t>(bitsLeftToRead);
      rawresult |= (curByte >> bitsLeft) & ((1 << bitsLeftToRead) - 1);
    }
    if(issigned){
    	result[i] = unZigZag(rawresult);
    }else{
        result[i] = static_cast<int64_t>(rawresult);
    }
  }

  return len;
}


uint64_t RLE2::nextShortRepeats(int64_t *result, uint64_t offset){

	unsigned char firstByte = data[index++];
	uint64_t byteSize = (firstByte >> 3) & 0x07;
	byteSize += 1;

	uint64_t runLength = firstByte & 0x07;
	// run lengths values are stored only after MIN_REPEAT value is met
	runLength += MIN_REPEAT;
	// read the repeated value which is store using fixed bytes
	int64_t value = readLongBE(byteSize);

	if(issigned){
		value = unZigZag(static_cast<uint64_t>(value));
	}
	//TODO modify for run time code generation
	for(uint64_t pos = offset; pos < runLength+offset; ++pos) {
	      result[pos] = value;
	}
	return runLength;
}

uint64_t RLE2::nextPatched(int64_t *result, uint64_t offset){

	unsigned char firstbyte = data[index++];
	unsigned char secondbyte = data[index++];
	unsigned char thirdbyte = data[index++];
	unsigned char fourthbyte = data[index++];

	unsigned char fbo = (firstbyte >> 1) & 0x1f;
	uint64_t bitSize = decodeBitWidth(fbo);

	// extract the run length
	uint64_t runLength = static_cast<uint64_t>(firstbyte & 0x01) << 8;
	runLength |= secondbyte;
	// runs are one off
	runLength += 1;

	std::cout<<((firstbyte >> 6) & 0x03)<<" "<<bitSize<<" "<<runLength<<std::endl;
	// extract the number of bytes occupied by base
	uint64_t byteSize = (thirdbyte >> 5) & 0x07;
	// base width is one off
	byteSize += 1;

	// extract patch width
	uint32_t pwo = thirdbyte & 0x1f;
	uint32_t patchBitSize = decodeBitWidth(pwo);

	// read fourth byte and extract patch gap width
	uint32_t pgw = (fourthbyte >> 5) & 0x07;
	// patch gap width is one off
	pgw += 1;

	// extract the length of the patch list
	size_t pl = fourthbyte & 0x1f;
	if (pl == 0) {
	    std::cerr<<"Corrupt PATCHED_BASE encoded data (pl==0)!\n";
	}
	exit(0);
//
//	    // read the next base width number of bytes to extract base value
//	    base = readLongBE(byteSize);
//	    int64_t mask = (static_cast<int64_t>(1) << ((byteSize * 8) - 1));
//	    // if mask of base value is 1 then base is negative value else positive
//	    if ((base & mask) != 0) {
//	      base = base & ~mask;
//	      base = -base;
//	    }
//
//	    // TODO: something more efficient than resize
//	    unpacked.resize(runLength);
//	    unpackedIdx = 0;
//	    readLongs(unpacked.data(), 0, runLength, bitSize);
//	    // any remaining bits are thrown out
//	    resetReadLongs();
//
//	    // TODO: something more efficient than resize
//	    unpackedPatch.resize(pl);
//	    patchIdx = 0;
//	    // TODO: Skip corrupt?
//	    //    if ((patchBitSize + pgw) > 64 && !skipCorrupt) {
//	    if ((patchBitSize + pgw) > 64) {
//	      throw ParseError("Corrupt PATCHED_BASE encoded data "
//	                       "(patchBitSize + pgw > 64)!");
//	    }
//	    uint32_t cfb = getClosestFixedBits(patchBitSize + pgw);
//	    readLongs(unpackedPatch.data(), 0, pl, cfb);
//	    // any remaining bits are thrown out
//	    resetReadLongs();
//
//	    // apply the patch directly w -O3hen decoding the packed data
//	    patchMask = ((static_cast<int64_t>(1) << patchBitSize) - 1);
//
//	    adjustGapAndPatch();
	return 0;
}

uint64_t RLE2::nextDelta(int64_t *result, uint64_t offset){

	unsigned char firstByte = data[index++];
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
	if(issigned){
		firstValue = static_cast<int64_t>(readVslong());
	}else{
		firstValue = static_cast<int64_t>(readVulong());
	}

	// read the fixed delta value stored as vint (deltas can be negative even
	// if all number are positive)
	int64_t deltaBase = static_cast<int64_t>(readVslong());

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
	    readLongs(result, pos, remaining, bitSize);

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

uint64_t RLE2::nextDirect(int64_t *result, uint64_t offset){

	unsigned char firstbyte = data[index++];
	unsigned char secondbyte = data[index++];
	uint32_t  bitSize = decodeBitWidth(((firstbyte >> 1) & 0x1f));
	uint64_t runLength = static_cast<uint64_t>(firstbyte & 0x01) << 8;
	runLength |= secondbyte;
	runLength += 1;

	readLongs(result, offset, runLength,bitSize);
	return runLength;
}

RLE2::RLE2(unsigned char *data, uint64_t datasize, bool issigned):
		data(data),datasize(datasize),issigned(issigned){
	index = 0;

}

RLE2::~RLE2(){

}



uint64_t count[4];
void RLE2::read(int64_t *result) {

  uint64_t offset = 0;

  while (index < datasize) {

	unsigned char firstByte = data[index];

    EncodingType enc = static_cast<EncodingType>
        ((firstByte >> 6) & 0x03);

    count[enc]++;
    switch(static_cast<int64_t>(enc)) {
    case SHORT_REPEAT:
      //printf("\nrepeat: %ld\n",count[enc]);
      offset += nextShortRepeats(result, offset);
      break;
    case DIRECT:
      //printf("\ndirect: %ld\n",count[enc]);
      offset += nextDirect(result, offset);
      break;
    case PATCHED_BASE:
      printf("patched: %ld\n",count[enc]);
      exit(0);
      //nRead += nextPatched(data, offset, length, notNull);
      break;
    case DELTA:
      offset += nextDelta(result, offset);
      break;
    default:
      std::cerr<<"unknown encoding\n";
    }
  }

 std::cout<<count[0]<<" "<<count[1]<<" "<<count[2]<<" "<<count[3]<<std::endl;

}

void RLE2::processRepeat(EncodingInfo &info){

	unsigned char firstByte = data[index++];
	uint64_t byteSize = (firstByte >> 3) & 0x07;
	byteSize += 1;
	index += byteSize;
	if(info.bitwidth<0){
		info.isbitwidthfixed = true;
		info.bitwidth = byteSize;
	}
	if(info.isbitwidthfixed && info.bitwidth!=byteSize){
		info.isbitwidthfixed = false;
	}
}

void RLE2::processDelta(EncodingInfo &info){

	unsigned char firstByte = data[index++];
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

	int pos = 1;
	unsigned char b;
	do{
		b = data[index++];
	}while(b>0x80);
	do{
		b = data[index++];
	}while(b>0x80);
	if (bitSize != 0){
		if (pos < runLength) {
		  pos++;
		}
		uint64_t remaining = runLength - pos;
		index += (remaining*bitSize)/8;
		if((remaining*bitSize)%8!=0){
			index++;
		}
	}
	if(info.bitwidth<0){
		info.isbitwidthfixed = true;
		info.bitwidth = bitSize;
	}
	if(info.isbitwidthfixed && info.bitwidth != bitSize){
		info.isbitwidthfixed = false;
	}
}

void RLE2::processDirect(EncodingInfo &info){


	unsigned char firstbyte = data[index++];
	unsigned char secondbyte = data[index++];
	uint32_t bitSize = decodeBitWidth(((firstbyte >> 1) & 0x1f));
	uint64_t runLength = static_cast<uint64_t>(firstbyte & 0x01) << 8;
	runLength |= secondbyte;
	runLength += 1;

	index += (runLength*bitSize)/8;
	if((runLength*bitSize)%8!=0){
		index++;
	}

	if(info.bitwidth<0){
		info.isbitwidthfixed = true;
		info.bitwidth = bitSize;
	}
	if(info.isbitwidthfixed && info.bitwidth != bitSize){
		info.isbitwidthfixed = false;
	}

}
void RLE2::getInfo(ColumnInfo &cinfo){

	uint64_t formerindex = this->index;
	uint32_t bitSize;
	uint64_t runLength;

	while(index<datasize){
		unsigned char firstByte = data[index];

		EncodingType enc = static_cast<EncodingType>
			((firstByte >> 6) & 0x03);

		count[enc]++;
		switch(static_cast<int64_t>(enc)) {
		case SHORT_REPEAT:
		  //printf("\nrepeat: %ld\n",count[enc]);
		  cinfo.hasrepeat = true;
		  processRepeat(cinfo.repeatInfo);
		  break;
		case DIRECT:
		  //printf("\ndirect: %ld\n",count[enc]);
		  cinfo.hasdirect = true;
		  processDirect(cinfo.directInfo);
		  break;
		case PATCHED_BASE:
			printf("patched: %ld\n",count[enc]);
			exit(0);
		  //nRead += nextPatched(data, offset, length, notNull);
		  break;
		case DELTA:
			cinfo.hasdelta = true;
			processDelta(cinfo.deltaInfo);
		  break;
		default:
		  std::cerr<<"unknown encoding\n";
		}
	};
	//std::cout<<count[0]<<" "<<count[1]<<" "<<count[2]<<" "<<count[3]<<std::endl;


	this->index = formerindex;
	if(cinfo.hasrepeat){
		cinfo.repeatInfo.issigned = this->issigned;
	}else if(cinfo.hasdirect){
		cinfo.directInfo.issigned = this->issigned;
	}else if(cinfo.hasdelta){
		cinfo.deltaInfo.issigned = this->issigned;
	}

}


}
