/*
 * test.cc
 *
 *  Created on: Dec 2, 2016
 *      Author: teng
 */

#include <immintrin.h>



void test(int *result){

	_mm256_storeu_si256((__m256i *)&result[0],_mm256_add_epi32(*(__m256i *)&result[0],*(__m256i *)&result[0]));

}
