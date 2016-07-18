/*
 * ColumnInfo.h
 *
 *  Created on: Jul 10, 2016
 *      Author: teng
 */

#ifndef SRC_COLUMNINFO_H_
#define SRC_COLUMNINFO_H_



struct EncodingInfo{
	bool issigned;
	bool isbitwidthfixed;
	uint64_t bitwidth;
	bool isrunlengthfixed;
	uint64_t runlength;

	EncodingInfo(){
		issigned = false;
		isbitwidthfixed = false;
		isrunlengthfixed = false;
		bitwidth = 0;
		runlength = 0;
	}

};
struct ColumnInfo{

	bool hasdirect;
	bool hasrepeat;
	bool haspatched;
	bool hasdelta;
	EncodingInfo directInfo;
	EncodingInfo repeatInfo;
	EncodingInfo patchedInfo;
	EncodingInfo deltaInfo;

};



#endif /* SRC_COLUMNINFO_H_ */
