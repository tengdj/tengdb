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
	int64_t bitwidth;
	bool isrunlengthfixed;
	int64_t runlength;

	EncodingInfo(){
		issigned = false;
		isbitwidthfixed = false;
		isrunlengthfixed = false;
		bitwidth = -1;
		runlength = -1;
	}

};
struct ColumnInfo{

	std::string colname;
	bool updateAggregation;
	bool saveResult;
	bool hasdirect;
	bool hasrepeat;
	bool haspatched;
	bool hasdelta;
	EncodingInfo directInfo;
	EncodingInfo repeatInfo;
	EncodingInfo patchedInfo;
	EncodingInfo deltaInfo;

	ColumnInfo(){
		hasdirect = false;
		hasrepeat = false;
		haspatched = false;
		hasdelta = false;
		updateAggregation = false;
		saveResult = false;
	}

};



#endif /* SRC_COLUMNINFO_H_ */
