/*
 * nextPatched.cc
 *
 *  Created on: Nov 8, 2016
 *      Author: teng
 */
#include <memory>
#include <stdint.h>

uint32_t closestFixedBits[57] =
	{1,
	1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
	26,26,
	28,28,
	30,30,
	32,32,
	40,40,40,40,40,40,40,40,
	48,48,48,48,48,48,48,48,
	56,56,56,56,56,56,56,56};
uint32_t decodeBitWidth(uint32_t n);
int64_t readLongBE(char *data, uint64_t &index, uint64_t bsz);
uint64_t readLongs(char *data, int64_t *result, uint64_t &index, uint64_t offset, uint64_t len, uint64_t fb);

uint64_t nextPatched(char *data, int64_t *result, uint64_t &index, uint64_t offset){

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

	//std::cout<<((firstbyte >> 6) & 0x03)<<" "<<bitSize<<" "<<runLength<<std::endl;
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
	uint32_t pl = fourthbyte & 0x1f;

	// read the next base width number of bytes to extract base value
	int64_t base = readLongBE(data, index, byteSize);
	int64_t mask = (static_cast<int64_t>(1) << ((byteSize * 8) - 1));
	// if mask of base value is 1 then base is negative value else positive
	if ((base & mask) != 0) {
	  base = base & ~mask;
	  base = -base;
	}

	int64_t unpacked[runLength];
	int64_t unpackedPatch[pl];
	//DataBuffer<int64_t> unpacked;
	//DataBuffer<int64_t> unpackedPatch;

	// TODO: something more efficient than resize
	//unpacked.resize(runLength);
	uint64_t unpackedIdx = 0;
	readLongs(data, unpacked, index, 0, runLength, bitSize);
	// any remaining bits are thrown out

	// TODO: something more efficient than resize
	//unpackedPatch.resize(pl);
	uint64_t patchIdx = 0;

	uint32_t cfb = closestFixedBits[patchBitSize + pgw];
	readLongs(data,unpackedPatch, index, 0, pl, cfb);
	// any remaining bits are thrown out

	// apply the patch directly w -O3hen decoding the packed data
	const int64_t patchMask = ((static_cast<int64_t>(1) << patchBitSize) - 1);

	uint64_t curGap = static_cast<uint64_t>(unpackedPatch[patchIdx]) >>  patchBitSize;
	int64_t curPatch = unpackedPatch[patchIdx] & patchMask;
	int64_t actualGap = 0;

	// special case: gap is >255 then patch value will be 0.
	// if gap is <=255 then patch value cannot be 0
	while (curGap == 255 && curPatch == 0) {
	  actualGap += 255;
	  ++patchIdx;
	  curGap = static_cast<uint64_t>(unpackedPatch[patchIdx]) >>
		patchBitSize;
	  curPatch = unpackedPatch[patchIdx] & patchMask;
	}
	// add the left over gap
	actualGap += curGap;

  for(uint64_t pos = offset; pos < offset + runLength; ++pos) {

	if (static_cast<int64_t>(unpackedIdx) != actualGap) {
	  // no patching required. add base to unpacked value to get final value
	  result[pos] = base + unpacked[unpackedIdx];
	} else {
	  // extract the patch value
	  int64_t patchedVal = unpacked[unpackedIdx] | (curPatch << bitSize);

	  // add base to patched value
	  result[pos] = base + patchedVal;

	  // increment the patch to point to next entry in patch list
	  ++patchIdx;

	  if (patchIdx < pl) {
		  curGap = static_cast<uint64_t>(unpackedPatch[patchIdx]) >>  patchBitSize;
		  curPatch = unpackedPatch[patchIdx] & patchMask;
		  actualGap = 0;

		  // special case: gap is >255 then patch value will be 0.
		  // if gap is <=255 then patch value cannot be 0
		  while (curGap == 255 && curPatch == 0) {
		  	 actualGap += 255;
		  	 ++patchIdx;
		  	 curGap = static_cast<uint64_t>(unpackedPatch[patchIdx]) >> patchBitSize;
		  	 curPatch = unpackedPatch[patchIdx] & patchMask;
		  }
		  // add the left over gap
		  actualGap += curGap;
		  // next gap is relative to the current gap
		  actualGap += unpackedIdx;
	  }
	}
	++unpackedIdx;
  }

  return runLength;
}
