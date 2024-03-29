/*
 * util.hh
 *
 *  Created on: Jun 26, 2016
 *      Author: teng
 */

#ifndef INCLUDE_TENG_UTIL_H_
#define INCLUDE_TENG_UTIL_H_

#include <stdio.h>
#include <unistd.h>
#include <string>
#include <mutex>
#include <dirent.h>
#include <vector>
#include <stdlib.h>
#include <map>
#include <memory>
#include <sys/stat.h>

#include "config.h"
#include "ORCReader.h"

namespace tengdb{

enum TypeKind {
    BOOLEAN = 0,
    BYTE = 1,//8
    SHORT = 2,//16
    INT = 3,//32
    LONG = 4,//64
    FLOAT = 5,//32
    DOUBLE = 6,//64
    STRING = 7,
    BINARY = 8,
    TIMESTAMP = 9,
    LIST = 10,
    MAP = 11,
    STRUCT = 12,
    UNION = 13,
    DECIMAL = 14,
    DATE = 15,
    VARCHAR = 16,
    CHAR = 17
};

inline int bitofType(TypeKind kind){
	switch(kind){
	case BOOLEAN:
		return 1;
	case BYTE:
		return 8;
	case DATE:
	case SHORT:
		return 16;
	case INT:
	case FLOAT:
	case STRING:
	case VARCHAR:
		return 32;
	case LONG:
	case DOUBLE:
		return 64;
	default:
		printf("variable length!\n");
		exit(0);
	}
}

inline int byteofType(TypeKind kind){
	int bit = bitofType(kind);
	if(bit<8){
		return 1;
	}else{
		return bit/8;
	}
}

struct FixedBitSizes {
  enum FBS {
    ONE = 0, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE,
    THIRTEEN, FOURTEEN, FIFTEEN, SIXTEEN, SEVENTEEN, EIGHTEEN, NINETEEN,
    TWENTY, TWENTYONE, TWENTYTWO, TWENTYTHREE, TWENTYFOUR, TWENTYSIX,
    TWENTYEIGHT, THIRTY, THIRTYTWO, FORTY, FORTYEIGHT, FIFTYSIX, SIXTYFOUR
  };
};

inline uint32_t getClosestFixedBits(uint32_t n) {
  if (n == 0) {
    return 1;
  }

  if (n >= 1 && n <= 24) {
    return n;
  } else if (n > 24 && n <= 26) {
    return 26;
  } else if (n > 26 && n <= 28) {
    return 28;
  } else if (n > 28 && n <= 30) {
    return 30;
  } else if (n > 30 && n <= 32) {
    return 32;
  } else if (n > 32 && n <= 40) {
    return 40;
  } else if (n > 40 && n <= 48) {
    return 48;
  } else if (n > 48 && n <= 56) {
    return 56;
  } else {
    return 64;
  }
}

inline uint32_t decodeBitWidth(uint32_t n) {
  if (n <= FixedBitSizes::TWENTYFOUR) {
    return n + 1;
  } else if (n == FixedBitSizes::TWENTYSIX) {
    return 26;
  } else if (n == FixedBitSizes::TWENTYEIGHT) {
    return 28;
  } else if (n == FixedBitSizes::THIRTY) {
    return 30;
  } else if (n == FixedBitSizes::THIRTYTWO) {
    return 32;
  } else if (n == FixedBitSizes::FORTY) {
    return 40;
  } else if (n == FixedBitSizes::FORTYEIGHT) {
    return 48;
  } else if (n == FixedBitSizes::FIFTYSIX) {
    return 56;
  } else {
    return 64;
  }
}

template <typename IT,typename UT>
inline IT unZigZag(UT value) {
    return value >> 1 ^ -(value & 1);
};

inline bool exists_module_file (const std::string& name) {
  struct stat buffer;
  return (stat ((name+"_opt.ll").c_str(), &buffer) == 0);
}

const static bool littleendian = true;


template<typename T>
inline int Encode(char* buf, T value) {
    memcpy(buf, &value, sizeof(T));
    return sizeof(T);;
}

template<typename T>
inline int Decode(char* buf, T *value) {
	memcpy(value, buf, sizeof(T));
	return sizeof(T);
}

template<typename T>
inline int EncodeArray(char *byte, T *data, int length){

	for(int i=0;i<length;i++){
		Encode(byte,data[i]);
		byte += sizeof(T);
	}
	return length*sizeof(T);

}

template<typename T>
inline int DecodeArray(char *byte, T *data, int length){

	for(int i=0;i<length;i++){
		Decode(byte,data);
		byte += sizeof(T);
		data++;

	}
	return length*sizeof(T);

}

inline int EncodeString(char* buf, std::string str) {
	Encode(buf,str.size());
	buf += 4;
	memcpy(buf,str.data(),str.size());
	return 4+str.size();
}

inline int DecodeString(char* buf, string &str) {
	int length;
	Decode(buf,&length);
	buf += 4;
	char s[length];
	memcpy(s,buf,length);
	str = std::string(s,length);;

	return 4+length;
}

}
#endif /* INCLUDE_UTIL_H_ */
