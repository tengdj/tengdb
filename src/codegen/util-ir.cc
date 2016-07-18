/*
 * util-ir.cc
 *
 *  Created on: Jul 12, 2016
 *      Author: teng
 */

#include <stdint.h>

uint32_t decodeBitWidth(uint32_t n) {
  if (n <= 23) {
    return n + 1;
  } else if (n == 24) {
    return 26;
  } else if (n == 25) {
    return 28;
  } else if (n == 26) {
    return 30;
  } else if (n == 27) {
    return 32;
  } else if (n == 28) {
    return 40;
  } else if (n == 29) {
    return 48;
  } else if (n == 30) {
    return 56;
  } else {
    return 64;
  }
}

int64_t unZigZag(uint64_t value) {
    return value >> 1 ^ -(value & 1);
};

int64_t readLongBE(char *data, uint64_t &index, uint64_t bsz) {
  int64_t ret = 0;
  uint64_t n = bsz;
  while (n > 0) {
    n--;
    ret |= (data[index++] << (n * 8));
  }
  return ret;
}


uint64_t readVulong(char *data, uint64_t &index) {
  uint64_t ret = 0, b;
  uint64_t offset = 0;
  do {
    b = data[index++];
    ret |= (0x7f & b) << offset;
    offset += 7;
  } while (b >= 0x80);
  return ret;
}

int64_t readVslong(char *data, uint64_t &index) {
  return unZigZag(readVulong(data, index));
}


uint64_t readLongs(char *data, int64_t *result, uint64_t &index, uint64_t offset, uint64_t len, uint64_t fb) {

  uint64_t bitsLeft = 0;
  char curByte;
  // TODO: unroll to improve performance
  for(uint64_t i = offset; i < (offset + len); i++) {

    uint64_t rawresult = 0;
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
    result[i] = unZigZag(rawresult);

  }

  return len;
}



