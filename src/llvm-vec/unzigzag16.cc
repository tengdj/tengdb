/*
 * TestVec.cpp
 *
 *  Created on: Dec 8, 2016
 *      Author: teng
 */

#include <immintrin.h>

void shuffle128(unsigned int *result, unsigned char *byte){

    __m128i mask = _mm_set_epi8(-1,9,10,11,-1,6,7,8,-1,3,4,5,-1,0,1,2);
    __m128i shuffle_value = _mm_shuffle_epi8(_mm_loadu_si128((__m128i *)&byte[0]), mask);
    _mm_store_si128((__m128i *)&result[0],shuffle_value);

}
