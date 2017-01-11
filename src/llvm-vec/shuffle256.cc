/*
 * TestVec.cpp
 *
 *  Created on: Dec 8, 2016
 *      Author: teng
 */

#include <immintrin.h>
#include <stdio.h>


void shuffle256(unsigned int result[], unsigned char byte[]){

#ifdef __AVX2__


	__m256i mask0 = _mm256_set_epi32(6,5,4,3,3,2,1,0);
    __m256i mask1 = _mm256_set_epi8(-1,9,10,11,-1,6,7,8,-1,3,4,5,-1,0,1,2,-1,9,10,11,-1,6,7,8,-1,3,4,5,-1,0,1,2);
    __m256i byte_value = _mm256_loadu_si256((__m256i *)&byte[0]);
    __m256i permute_var = _mm256_permutevar8x32_epi32(byte_value, mask0);
    __m256i shuffle_value = _mm256_shuffle_epi8(permute_var, mask1);
	_mm256_storeu_si256((__m256i *)&result[0], shuffle_value);

#endif

}

