/*
 * RLE2.hh
 *
 *  Created on: Jun 29, 2016
 *      Author: teng
 */

#ifndef INCLUDE_RLE2_H_
#define INCLUDE_RLE2_H_

#include <memory>
#include "ColumnInfo.h"
namespace orc{

class RLE2{

	unsigned char *data;
	uint64_t index;
	uint64_t datasize;
	bool issigned;

	uint64_t nextDirect(int64_t *result, uint64_t offset);
	uint64_t nextShortRepeats(int64_t *result, uint64_t offset);
	uint64_t nextDelta(int64_t *result, uint64_t offset);
	uint64_t nextPatched(int64_t *result, uint64_t offset);

	int64_t readLongBE(uint64_t bsz);
	uint64_t readVulong();
	int64_t readVslong();
	uint64_t readLongs(int64_t *result, uint64_t offset, uint64_t len, uint64_t fb);
	void processDirect(EncodingInfo &Info);
	void processDelta(EncodingInfo &Info);
	void processRepeat(EncodingInfo &Info);

public:
	void read(int64_t *result);
	RLE2(unsigned char *data, uint64_t datasize, bool issigned);
	void getInfo(ColumnInfo &cinfo);


	~RLE2();
};

void readDirect_spec(char *data, int64_t *result, uint64_t &index, uint64_t);
void readDirect_general(char *data, int64_t *result, uint64_t &index, uint64_t);
uint64_t readDirect_spec2(char *data, int64_t *result, uint64_t &index);
enum EncodingType { SHORT_REPEAT=0, DIRECT=1, PATCHED_BASE=2, DELTA=3 };


}
#endif /* INCLUDE_RLE2_H_ */
