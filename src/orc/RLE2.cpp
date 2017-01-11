/*
 * RLE2.cpp
 *
 *  Created on: Jun 29, 2016
 *      Author: teng
 */

#include "../include/RLE2.h"

#include <stdint.h>
#include <iostream>
#include <bitset>

#include "../include/util.h"
#include "../include/MemoryPool.h"
#include "config.h"
#include <immintrin.h>

#define MIN_REPEAT 3

using namespace std;
namespace orc{

template<typename I, typename UI>
I RLE2<I,UI>::readLongBE(uint64_t bsz) {
  I ret = 0, val;
  uint64_t n = bsz;
  while (n > 0) {
    n--;
    val = data[index++];
    //cout<<(unsigned int)val<<" ";
    ret |= (val << (n * 8));
  }
  //cout<<endl;
  return ret;
}

template<typename I, typename UI>
UI RLE2<I,UI>::readVulong() {
  UI ret = 0, b;
  uint64_t offset = 0;
  do {
    b = data[index++];
    ret |= (0x7f & b) << offset;
    offset += 7;
  } while (b >= 0x80);
  return ret;
}

template <typename I,typename UI>
inline I RLE2<I,UI>::unZigZag(UI value) {
    return value >> 1 ^ -(value & 1);
};

template<typename I, typename UI>
inline I RLE2<I,UI>::readVslong() {
  return unZigZag(readVulong());
}

//__m128i mask = _mm_set_epi8(-1,9,10,11,-1,6,7,8,-1,3,4,5,-1,0,1,2);
//
////__m128i mask = _mm_set_epi8(-1,-1,-1,-1,-1,3,4,5,-1,-1,-1,-1,-1,0,1,2);
//
//uint64_t RLE2::readLongs(int32_t *result, uint64_t offset, uint64_t len, uint64_t fb) {
//
//	uint64_t bitsLeft = 0;
//	unsigned char curByte;
//
//	uint one[4];
//	one[0] = 1;
//	one[1] = 1;
//	one[2] = 1;
//	one[3] = 1;
//
//	uint zero[4];
//	zero[0] = 0;
//	zero[1] = 0;
//	zero[2] = 0;
//	zero[3] = 0;
//
//	if(fb==24&&len==512){
//
//		uint64_t end = offset + len;
//	    for (; offset < end; offset += 4) {
//	        _mm_storeu_si128((__m128i *)&result[offset],
//	        		_mm_shuffle_epi8(_mm_loadu_si128((__m128i *)&data[index]), mask));
//
//	        //_mm_storeu_si128((__m128i *)&tmpresult[0],_mm_srai_epi32(*(__m128i *)&tmpresult[0],1));
//
//			_mm_storeu_si128((__m128i *)&result[offset],
//								_mm_xor_si128(
//										_mm_srai_epi32(*(__m128i *)&result[offset],1),
//										_mm_sub_epi32(*(__m128i *)&zero[0],_mm_and_si128(*(__m128i *)&result[offset],*(__m128i *)&one[0]))));
//	        index += 12;
//	    }
//
//	}else{
//	// TODO: unroll to improve performance
//	for(uint64_t i = offset; i < (offset + len); i++) {
//
//		uint32_t rawresult = 0;
//		uint32_t bitsLeftToRead = fb;
//
//		if(bitsLeft==0){
//			curByte = data[index++];
//			bitsLeft = 8;
//		}
//		while (bitsLeftToRead > bitsLeft){
//
//			rawresult <<= bitsLeft;
//			rawresult |= curByte & ((1 << bitsLeft) - 1);
//			bitsLeftToRead -= bitsLeft;
//			curByte = data[index++];
//			bitsLeft = 8;
//		}
//		// handle the left over bits
//		if (bitsLeftToRead > 0){
//
//		  rawresult <<= bitsLeftToRead;
//		  bitsLeft -= static_cast<uint32_t>(bitsLeftToRead);
//		  rawresult |= (curByte >> bitsLeft) & ((1 << bitsLeftToRead) - 1);
//		}
//		if(issigned){
//			result[i] = unZigZag(rawresult);
//		}else{
//			result[i] = static_cast<int32_t>(rawresult);
//		}
//	}
//  }
//
//
//  return len;
//}

#ifdef __AVX2__

__m256i mask0_32to32 = _mm256_set_epi32(7,6,5,4,3,2,1,0);
__m256i mask1_32to32 = _mm256_set_epi8(12,13,14,15,8,9,10,11,4,5,6,7,0,1,2,3,12,13,14,15,8,9,10,11,4,5,6,7,0,1,2,3);

__m256i mask0_24to32 = _mm256_set_epi32(6,5,4,3,3,2,1,0);
__m256i mask1_24to32 = _mm256_set_epi8(-1,9,10,11,-1,6,7,8,-1,3,4,5,-1,0,1,2,-1,9,10,11,-1,6,7,8,-1,3,4,5,-1,0,1,2);

__m256i mask0_16to32 = _mm256_set_epi32(5,4,3,2,3,2,1,0);
__m256i mask1_16to32 = _mm256_set_epi8(-1,-1,6,7,-1,-1,4,5,-1,-1,2,3,-1,-1,0,1,-1,-1,6,7,-1,-1,4,5,-1,-1,2,3,-1,-1,0,1);

__m256i mask0_8to32 = _mm256_set_epi32(3,2,1,0,3,2,1,0);
__m256i mask1_8to32 = _mm256_set_epi8(-1,-1,-1,7,-1, -1,-1,6,-1, -1,-1,5,-1, -1,-1,4,-1, -1,-1,3,-1, -1,-1,2,-1, -1,-1,1,-1, -1,-1,0);

__m256i mask0_4to32 = _mm256_set_epi32(3,2,1,0,3,2,1,0);
__m256i mask1_4to32 = _mm256_set_epi8(-1,-1,-1,3,-1, -1,-1,3,-1, -1,-1,2,-1, -1,-1,2,-1, -1,-1,1,-1, -1,-1,1,-1, -1,-1,0,-1, -1,-1,0);
__m256i shift_4to32 = _mm256_set_epi32(0,4,0,4,0,4,0,4);
__m256i and_4to32 = _mm256_set_epi32(31,31,31,31,31,31,31,31);

__m256i mask0_2to32 = _mm256_set_epi32(0,0,1,0,0,0,1,0);
__m256i mask1_2to32 = _mm256_set_epi8(-1,-1,-1,1,-1, -1,-1,1,-1, -1,-1,1,-1, -1,-1,1,-1, -1,-1,0,-1, -1,-1,0,-1, -1,-1,0,-1, -1,-1,0);
__m256i shift_2to32 = _mm256_set_epi32(0,2,4,6,0,2,4,6);
__m256i and_2to32 = _mm256_set_epi32(3,3,3,3,3,3,3,3);

__m256i mask0_16to16 = _mm256_set_epi32(7,6,5,4,3,2,1,0);
__m256i mask1_16to16 = _mm256_set_epi8(14,15,12,13,10,11,8,9,6,7,4,5,2,3,0,1,14,15,12,13,10,11,8,9,6,7,4,5,2,3,0,1);

__m256i mask0_8to16 = _mm256_set_epi32(3,2,1,0,3,2,1,0);
__m256i mask1_8to16 = _mm256_set_epi8(-1,15,-1,14,-1,13,-1,12,-1,11,-1,10,-1,9,-1,8,-1,7,-1,6,-1,5,-1,4,-1,3,-1,2,-1,1,-1,0);

__m256i mask0_4to16 = _mm256_set_epi32(3,2,1,0,3,2,1,0);
__m256i mask1_4to16 = _mm256_set_epi8(-1,7,-1,7,-1,6,-1,6,-1,5,-1,5,-1,4,-1,4,-1,3,-1, 3,-1, 2,-1,2,-1,1,-1,1,-1, 0,-1,0);

__m256i mask0_8to8 = _mm256_set_epi32(7,6,5,4,3,2,1,0);
__m256i mask1_8to8 = _mm256_set_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);

__m256i mask0_4to8 = _mm256_set_epi32(3,2,1,0,3,2,1,0);
__m256i mask1_4to8 = _mm256_set_epi8(15,15,14,14,13,13,12,12,1,11,10,10,9,9,8,8,7,7,6,6,5,5,4,4,3,3,2,2,1,1,0,0);


__m256i one_32 = _mm256_set_epi32(1,1,1,1,1,1,1,1);
__m256i zero_32 = _mm256_set_epi32(0,0,0,0,0,0,0,0);

__m256i one_16 = _mm256_set_epi8(0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1);
__m256i zero_16 = _mm256_set_epi32(0,0,0,0,0,0,0,0);

inline __m256i unZigZag32(__m256i result){

	__m256i and_value = _mm256_and_si256(result,one_32);
	__m256i sub_value = _mm256_sub_epi32(zero_32,and_value);
	__m256i shift_value = _mm256_srai_epi32(result,1);
	return _mm256_xor_si256(shift_value,sub_value);

}

inline __m256i unZigZag16(__m256i result){

	__m256i and_value = _mm256_and_si256(result,one_16);
	__m256i sub_value = _mm256_sub_epi16(zero_16,and_value);
	__m256i shift_value = _mm256_srai_epi16(result,1);
	return _mm256_xor_si256(shift_value,sub_value);

}

#endif
template<typename I, typename UI>
uint64_t RLE2<I,UI>::readLongs(I *result, uint64_t offset, uint64_t len, uint64_t fb,bool issigned) {

	uint64_t bitsLeft = 0;
	unsigned char curByte;
	const uint64_t end = offset + len;

#ifdef __AVX2__

	int batch = 32/sizeof(I);
	int foldrun = len/batch;
	int bytesize = fb/8;

	if(orc::FLAGS_vectorization&&foldrun>0&&((fb>=8&&fb%8==0)||(fb<8&&8%fb==0))){
		if(sizeof(I)==4){
			//cout<<batch<<" "<<foldrun<<" "<<bytesize<<endl;
			if(bytesize==4){
				for(int j=0;j<foldrun;j++){
					__m256i data_256 = _mm256_loadu_si256((__m256i *)&data[index]);
					__m256i permute_value = _mm256_permutevar8x32_epi32(data_256, mask0_32to32);
					__m256i shuffled_value = _mm256_shuffle_epi8(permute_value, mask1_32to32);
					if(issigned){
						_mm256_storeu_si256((__m256i *)&result[offset], unZigZag32(shuffled_value));
					}else{
						_mm256_storeu_si256((__m256i *)&result[offset], shuffled_value);
					}
					offset += batch;
					index += batch*bytesize;
				}
			}else if(bytesize==3){
				for(int j=0;j<foldrun;j++){
					__m256i data_256 = _mm256_loadu_si256((__m256i *)&data[index]);
					__m256i permute_value = _mm256_permutevar8x32_epi32(data_256, mask0_24to32);
					__m256i shuffled_value = _mm256_shuffle_epi8(permute_value, mask1_24to32);
					if(issigned){
						_mm256_storeu_si256((__m256i *)&result[offset], unZigZag32(shuffled_value));
					}else{
						_mm256_storeu_si256((__m256i *)&result[offset], shuffled_value);
					}
					offset += batch;
					index += batch*bytesize;
				}
			}else if(bytesize == 2){
				for(int j=0;j<foldrun;j++){
					__m256i data_256 = _mm256_loadu_si256((__m256i *)&data[index]);
					__m256i permute_value = _mm256_permutevar8x32_epi32(data_256, mask0_16to32);
					__m256i shuffled_value = _mm256_shuffle_epi8(permute_value, mask1_16to32);
					if(issigned){
						_mm256_storeu_si256((__m256i *)&result[offset], unZigZag32(shuffled_value));
					}else{
						_mm256_storeu_si256((__m256i *)&result[offset], shuffled_value);
					}
					offset += batch;
					index += batch*bytesize;

				}
			}else if(bytesize==1){
				for(int j=0;j<foldrun;j++){
					__m256i data_256 = _mm256_loadu_si256((__m256i *)&data[index]);
					__m256i permute_value = _mm256_permutevar8x32_epi32(data_256, mask0_8to32);
					__m256i shuffled_value = _mm256_shuffle_epi8(permute_value, mask1_8to32);
					if(issigned){
						_mm256_storeu_si256((__m256i *)&result[offset], unZigZag32(shuffled_value));
					}else{
						_mm256_storeu_si256((__m256i *)&result[offset], shuffled_value);
					}
					offset += batch;
					index += batch*bytesize;
				}
			}else if(fb==4){
				for(int j=0;j<foldrun;j++){
					__m256i data_256 = _mm256_loadu_si256((__m256i *)&data[index]);
					__m256i permute_value = _mm256_permutevar8x32_epi32(data_256, mask0_4to32);
					__m256i shuffled_value = _mm256_shuffle_epi8(permute_value, mask1_4to32);
					__m256i shift_value = _mm256_srlv_epi32(shuffled_value, shift_4to32);
					__m256i and_value = _mm256_and_si256(shift_value, and_4to32);
					if(issigned){
						_mm256_storeu_si256((__m256i *)&result[offset], unZigZag32(and_value));
					}else{
						_mm256_storeu_si256((__m256i *)&result[offset], and_value);
					}
					offset += batch;
					index += batch/2;
				}

			}else if(fb==2){
				for(int j=0;j<foldrun;j++){
					__m256i data_256 = _mm256_loadu_si256((__m256i *)&data[index]);
					__m256i permute_value = _mm256_permutevar8x32_epi32(data_256, mask0_2to32);
					__m256i shuffled_value = _mm256_shuffle_epi8(permute_value, mask1_2to32);
					__m256i shift_value = _mm256_srlv_epi32(shuffled_value, shift_2to32);
					__m256i and_value = _mm256_and_si256(shift_value, and_2to32);
					if(issigned){
						_mm256_storeu_si256((__m256i *)&result[offset], unZigZag32(and_value));
					}else{
						_mm256_storeu_si256((__m256i *)&result[offset], and_value);
					}
					offset += batch;
					index += batch/4;
				}

			}
		}else if(sizeof(I)==2){
			if(bytesize==2){
				for(int j=0;j<foldrun;j++){
					__m256i data_256 = _mm256_loadu_si256((__m256i *)&data[index]);
					__m256i permute_value = _mm256_permutevar8x32_epi32(data_256, mask0_16to16);
					__m256i shuffled_value = _mm256_shuffle_epi8(permute_value, mask1_16to16);
					if(issigned){
						_mm256_storeu_si256((__m256i *)&result[offset], unZigZag16(shuffled_value));
					}else{
						_mm256_storeu_si256((__m256i *)&result[offset], shuffled_value);
					}
					offset += batch;
					index += batch*bytesize;
				}
			}else if(bytesize==1){
				for(int j=0;j<foldrun;j++){
					__m256i data_256 = _mm256_loadu_si256((__m256i *)&data[index]);
					__m256i permute_value = _mm256_permutevar8x32_epi32(data_256, mask0_8to16);
					__m256i shuffled_value = _mm256_shuffle_epi8(permute_value, mask1_8to16);
					if(issigned){
						_mm256_storeu_si256((__m256i *)&result[offset], unZigZag16(shuffled_value));
					}else{
						_mm256_storeu_si256((__m256i *)&result[offset], shuffled_value);
					}
					offset += batch;
					index += batch*bytesize;
				}
			}else if(fb==4){
				for(int j=0;j<foldrun;j++){
					__m256i data_256 = _mm256_loadu_si256((__m256i *)&data[index]);
					__m256i permute_value = _mm256_permutevar8x32_epi32(data_256, mask0_4to16);
					__m256i shuffled_value = _mm256_shuffle_epi8(permute_value, mask1_4to16);
					if(issigned){
						_mm256_storeu_si256((__m256i *)&result[offset], unZigZag16(shuffled_value));
					}else{
						_mm256_storeu_si256((__m256i *)&result[offset], shuffled_value);
					}
					offset += batch;
					index += batch/2;
				}

			}

		}else if(sizeof(I)==1){
			if(bytesize==1){
				for(int j=0;j<foldrun;j++){
					__m256i data_256 = _mm256_loadu_si256((__m256i *)&data[index]);
					__m256i permute_value = _mm256_permutevar8x32_epi32(data_256, mask0_8to8);
					__m256i shuffled_value = _mm256_shuffle_epi8(permute_value, mask1_8to8);
					_mm256_storeu_si256((__m256i *)&result[offset], shuffled_value);
					offset += batch;
					index += batch*bytesize;
				}
			}else if(fb==4){
				for(int j=0;j<foldrun;j++){
					__m256i data_256 = _mm256_loadu_si256((__m256i *)&data[index]);
					__m256i permute_value = _mm256_permutevar8x32_epi32(data_256, mask0_8to8);
					__m256i shuffled_value = _mm256_shuffle_epi8(permute_value, mask1_8to8);
					_mm256_storeu_si256((__m256i *)&result[offset], shuffled_value);
					offset += batch;
					index += batch/2;
				}

			}

		}

	}
#endif


	// TODO: unroll to improve performance
	for(uint64_t i = offset; i < end; i++) {

		UI rawresult = 0;
		uint64_t bitsLeftToRead = fb;

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
			result[i] = static_cast<I>(rawresult);
		}
	}



  return len;
}

static void updateEncodingInfo(EncodingInfo *info,uint64_t bsize, uint64_t runlength){

	if(!orc::FLAGS_codegen){
		return;
	}
	info->bitsize[bsize]++;
	info->runLength[runlength] += runlength;
	info->num++;
	info->totalRunLength += runlength;

}


template<typename I, typename UI>
uint64_t RLE2<I,UI>::nextDirect(I *result, uint64_t offset, EncodingInfo *info){

	unsigned char firstbyte = data[index++];
	unsigned char secondbyte = data[index++];
	uint32_t  bitSize = decodeBitWidth(((firstbyte >> 1) & 0x1f));
	uint64_t runLength = static_cast<uint64_t>(firstbyte & 0x01) << 8;
	runLength |= secondbyte;
	runLength += 1;

	readLongs(result, offset, runLength,bitSize,info->issigned);
	updateEncodingInfo(info,bitSize,runLength);
	return runLength;
}

template<typename I, typename UI>
uint64_t RLE2<I,UI>::nextShortRepeats(I *result, uint64_t offset,EncodingInfo *info){

	unsigned char firstByte = data[index++];
	uint64_t byteSize = (firstByte >> 3) & 0x07;
	byteSize += 1;

	uint64_t runLength = firstByte & 0x07;
	// run lengths values are stored only after MIN_REPEAT value is met
	runLength += MIN_REPEAT;

	uint64_t pos = offset;
	uint64_t end = offset + runLength;
	// read the repeated value which is store using fixed bytes
	I value = readLongBE(byteSize);

	if(issigned){
		value = unZigZag(static_cast<UI>(value));
	}

#ifdef __AVX2__
	if(orc::FLAGS_vectorization&&sizeof(I)==4){
		__m256i repeated_value = _mm256_set1_epi32(value);
		_mm256_storeu_si256((__m256i *)&result[pos], repeated_value);
		pos+=8;
	}
#endif
	//TODO modify for run time code generation
	for(; pos < end; ++pos) {
	      result[pos] = value;
	}

	updateEncodingInfo(info,byteSize,runLength);

	return runLength;
}

template<typename I, typename UI>
uint64_t RLE2<I,UI>::nextPatched(I *result, uint64_t offset,EncodingInfo *info){

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

	uint32_t cfb = getClosestFixedBits(patchBitSize + pgw);
	if (cfb > 64) {
	  printf("Corrupt PATCHED_BASE encoded data "
					   "(patchBitSize + pgw > 64)!\n");
	  exit(0);
	}

	// extract the length of the patch list
	size_t pl = fourthbyte & 0x1f;
	if (pl == 0) {
	    std::cerr<<"Corrupt PATCHED_BASE encoded data (pl==0)!\n";
	    exit(0);
	}

	// read the next base width number of bytes to extract base value
	I base = readLongBE(byteSize);
	I mask = (static_cast<I>(1) << ((byteSize * 8) - 1));
	// if mask of base value is 1 then base is negative value else positive
	if ((base & mask) != 0) {
	  base = base & ~mask;
	  base = -base;
	}

	I unpackedPatch[pl];


	readLongs(result+offset, 0, runLength, bitSize,false);
	readLongs(unpackedPatch, 0, pl, cfb,false);


	for(uint64_t pos = offset; pos < offset + runLength; ++pos) {
		result[pos] = base + result[pos];
	}

	const I patchMask = ((static_cast<I>(1) << patchBitSize) - 1);
	for(uint64_t i=0,pos=offset;i<pl;i++){
		pos += static_cast<uint64_t>(unpackedPatch[i]) >>  patchBitSize;
		I curPatch = unpackedPatch[i] & patchMask;
		I patchedVal = (result[pos] - base)| (curPatch << bitSize);
		result[pos] = base + patchedVal;
	}

    updateEncodingInfo(info,bitSize,runLength);
    return runLength;
}


template<typename I, typename UI>
uint64_t RLE2<I,UI>::nextDelta(I *result, uint64_t offset, EncodingInfo *info){

	unsigned char firstByte = data[index++];
	unsigned char secondByte = data[index++];
	// extract the number of fixed bits
	unsigned char fbo = (firstByte >> 1) & 0x1f;
	uint64_t bitSize = 0;
	uint64_t runLength;
	if (fbo != 0) {
	   bitSize = decodeBitWidth(fbo);
	}

	// extract the run length
	runLength = static_cast<uint64_t>(firstByte & 0x01) << 8;
	runLength |= secondByte;
	runLength += 1; // account for first value

	// read the first value stored as vint

	I firstValue;
	if(issigned){
		firstValue = static_cast<I>(readVslong());
	}else{
		firstValue = static_cast<I>(readVulong());
	}


	// read the fixed delta value stored as vint (deltas can be negative even
	// if all number are positive)
	I deltaBase = static_cast<I>(readVslong());

	uint64_t pos = offset;

	uint64_t end = offset + runLength;
	I prevValue = firstValue;

	result[pos++] = firstValue;

	if (bitSize == 0) {
	    // add fixed deltas to adjacent values
		if(deltaBase!=0){
#ifdef __AVX2__
//			if(orc::FLAGS_vectorization&&sizeof(I)==4){
//				pos -= 1;
//				int fold = runLength/8;
//				__m256i repeated_value = _mm256_set1_epi32(firstValue);
//				for(int i=0;i<fold;i++){
//					_mm256_storeu_si256((__m256i *)&result[pos], repeated_value);
//					pos+=8;
//				}
//			}
#endif
		    for ( ;pos < end;) {
		      prevValue = result[pos++] = prevValue + deltaBase;
		    }
		}else{
#ifdef __AVX2__

			if(orc::FLAGS_vectorization&&sizeof(I)==4){
				pos -= 1;
				int fold = runLength/8;
				__m256i repeated_value = _mm256_set1_epi32(firstValue);
				for(int i=0;i<fold;i++){
					_mm256_storeu_si256((__m256i *)&result[pos], repeated_value);
					pos+=8;
				}
			}
#endif
		    for ( ;pos < end;) {
		      result[pos++] = firstValue;
		    }
		}

	} else {
	    //if (pos < end)
	    {
	      // add delta base and first value
	      prevValue = result[pos++] = firstValue + deltaBase;

	    }

	    // write the unpacked values, add it to previous value and store final
	    // value to result buffer. if the delta base value is negative then it
	    // is a decreasing sequence else an increasing sequence
	    uint64_t remaining = end - pos;
	    readLongs(result, pos, remaining, bitSize, false);

	    if (deltaBase < 0) {
	      for ( ; pos < end; ++pos) {
	        prevValue = result[pos] = prevValue - result[pos];

	      }
	    } else {

#ifdef	__AVX2__

	    	if(orc::FLAGS_vectorization&&sizeof(I)==4&&runLength>8){


			__m256i permute1 = _mm256_set_epi32(6,5,4,3,2,1,0,0);
			__m256i mask1 = _mm256_set_epi32(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0);
			__m256i permute2 = _mm256_set_epi32(5,4,3,2,1,0,0,0);
			__m256i mask2 = _mm256_set_epi32(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0,0);
			__m256i permute4 = _mm256_set_epi32(3,2,1,0,0,0,0,0);
			__m256i mask4 = _mm256_set_epi32(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0,0,0,0);
			__m256i permute_fetch_top = _mm256_set_epi32(7,7,7,7,7,7,7,7);

			__m256i base = _mm256_setzero_si256();
			pos -= 2;
			result[pos+1] = deltaBase;
			uint64_t trueend = offset+(runLength/8)*8;
			for (; pos < trueend ; pos += 8) {

				__m256i x = _mm256_loadu_si256((__m256i *)&result[pos]);

				__m256i tmp = _mm256_permutevar8x32_epi32(x,permute1);
				tmp = _mm256_and_si256(tmp,mask1);
				x = _mm256_add_epi32(x,tmp);

				tmp = _mm256_permutevar8x32_epi32(x,permute2);
				tmp = _mm256_and_si256(tmp,mask2);
				x = _mm256_add_epi32(x,tmp);

				tmp = _mm256_permutevar8x32_epi32(x,permute4);
				tmp = _mm256_and_si256(tmp,mask4);
				x = _mm256_add_epi32(x,tmp);

				x = _mm256_add_epi32(x,base);

				_mm256_storeu_si256((__m256i *)&result[pos], x);
				base = _mm256_permutevar8x32_epi32(x,permute_fetch_top);

			}
		    prevValue = result[trueend-1];

	    }
#endif
	      for ( ; pos < end; ++pos) {
	        prevValue = result[pos] = prevValue + result[pos];
	      }
	    }
	  }

	updateEncodingInfo(info,bitSize,runLength);
	return runLength;
}


template<typename I, typename UI>
RLE2<I,UI>::RLE2(unsigned char *data, uint64_t datasize, bool issigned):
		data(data),datasize(datasize),issigned(issigned){
	index = 0;

}

template<typename I, typename UI>
RLE2<I,UI>::~RLE2(){

}

template<typename I, typename UI>
void RLE2<I,UI>::next(I *result,RLEInfo *cinfo) {

  uint64_t offset = 0;
  while (index < datasize) {

	unsigned char firstByte = data[index];

    EncodingType enc = static_cast<EncodingType>
        ((firstByte >> 6) & 0x03);

    switch(static_cast<int64_t>(enc)) {
    case SHORT_REPEAT:
      offset += nextShortRepeats(result, offset,cinfo->repeatInfo);
      break;
    case DIRECT:
      offset += nextDirect(result, offset,cinfo->directInfo);
      break;
    case PATCHED_BASE:
      offset += nextPatched(result, offset, cinfo->patchedInfo);
      break;
    case DELTA:
      offset += nextDelta(result, offset, cinfo->deltaInfo);
      break;
    default:
      std::cerr<<"unknown encoding\n";
    }
  }
}

template class RLE2<int64_t,uint64_t>;
template class RLE2<int32_t,uint32_t>;
template class RLE2<int16_t,uint16_t>;
template class RLE2<int8_t,uint8_t>;


}
