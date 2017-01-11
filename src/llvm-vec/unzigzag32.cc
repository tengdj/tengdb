/*
 * TestVec.cpp
 *
 *  Created on: Dec 8, 2016
 *      Author: teng
 */

#include <immintrin.h>

__m256i unZigZag32(__m256i result, __m256i one_32, __m256i zero_32){

	__m256i and_value = _mm256_and_si256(result,one_32);
	__m256i sub_value = _mm256_sub_epi32(zero_32,and_value);
	__m256i shift_value = _mm256_srai_epi32(result,1);
	return _mm256_xor_si256(shift_value,sub_value);

}
