/*
 * RLE2.hh
 *
 *  Created on: Jun 29, 2016
 *      Author: teng
 */

#ifndef INCLUDE_RLE2_H_
#define INCLUDE_RLE2_H_

#include <memory>

#include "ORCColumnInfo.h"

using namespace tengdb;
namespace orc{

template <typename I, typename UI>
class RLE2{

	unsigned char *data;
	uint64_t index;
	uint64_t datasize;
	bool issigned;

	uint64_t nextDirect(I *result, uint64_t offset,EncodingInfo *info);
	uint64_t nextShortRepeats(I *result, uint64_t offset,EncodingInfo *info);
	uint64_t nextDelta(I *result, uint64_t offset,EncodingInfo *info);
	uint64_t nextPatched(I *result, uint64_t offset,EncodingInfo *info);
	inline I unZigZag(UI value);
	I readLongBE(uint64_t bsz);
	UI readVulong();
	I readVslong();
	uint64_t readLongs(I *result, uint64_t offset, uint64_t len, uint64_t fb, bool issigned = false);

public:
	void next(I *result,RLEInfo *cinfo);
	RLE2(unsigned char *data, uint64_t datasize, bool issigned);

	~RLE2();
};

enum EncodingType {
	SHORT_REPEAT=0,
	DIRECT=1,
	PATCHED_BASE=2,
	DELTA=3
};


}
#endif /* INCLUDE_RLE2_H_ */
