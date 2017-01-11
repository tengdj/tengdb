/*
 * prescan.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: teng
 */

#include <immintrin.h>
#include <omp.h>
#include <iostream>
#include <stdlib.h>
using namespace std;

#ifdef	__AVX2__

__m256i permute1 = _mm256_set_epi32(6,5,4,3,2,1,0,0);
__m256i mask1 = _mm256_set_epi32(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0);
__m256i permute2 = _mm256_set_epi32(5,4,3,2,1,0,0,0);
__m256i mask2 = _mm256_set_epi32(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0,0);
__m256i permute4 = _mm256_set_epi32(3,2,1,0,0,0,0,0);
__m256i mask4 = _mm256_set_epi32(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0,0,0,0);
__m256i permute_fetch_top = _mm256_set_epi32(7,7,7,7,7,7,7,7);

#endif
void presum256(int a[], int s[], int l, int n) {

	for(int r = 0;r<n;r++){
		for(int i=0;i<l;i++){
				a[i] = i;
		}
#ifdef	__AVX2__
		__m256i offset = _mm256_setzero_si256();
		for (int i = 0; i < l ; i += 8) {

			__m256i x = _mm256_loadu_si256((__m256i *)&a[i]);

			__m256i tmp = _mm256_permutevar8x32_epi32(x,permute1);
			tmp = _mm256_and_si256(tmp,mask1);
			x = _mm256_add_epi32(x,tmp);

			tmp = _mm256_permutevar8x32_epi32(x,permute2);
			tmp = _mm256_and_si256(tmp,mask2);
			x = _mm256_add_epi32(x,tmp);

			tmp = _mm256_permutevar8x32_epi32(x,permute4);
			tmp = _mm256_and_si256(tmp,mask4);
			x = _mm256_add_epi32(x,tmp);

			x = _mm256_add_epi32(x,offset);

			_mm256_storeu_si256((__m256i *)&a[i], x);
			offset = _mm256_permutevar8x32_epi32(x,permute_fetch_top);

		}
#endif
	}

}

void presum128(int a[], int s[], int l, int n) {

	for(int r=0;r<n;r++){
		__m128i offset = _mm_setzero_si128();
		for (int i = 0; i < l / 4; i++) {
			__m128i x = _mm_load_si128((__m128i *)&a[4*i]);
			__m128i shift1 = _mm_slli_si128(x, 4);
			x = _mm_add_epi32(x, shift1);
			__m128i shift2 = _mm_slli_si128(x, 8);
			x = _mm_add_epi32(x, shift2);
			x = _mm_add_epi32(x, offset);
			_mm_store_si128((__m128i *)&s[4 * i], x);
			offset = _mm_shuffle_epi32(x,0xFF);
		}
	}


}

void presum(int a[], int s[], int l, int n){
	for(int r=0;r<n;r++){
		s[0] = a[0];
		for(int i=1;i<l;i++){
			s[i] = s[i-1]+a[i];
		}
	}

}
int main(int argc, char **argv){

	int runnum = 100*10000;
	int runlength = 512;
	int a[runlength];
	int s[runlength];
	//int *a = (int *)aligned_alloc(32,runlength*sizeof(int));
	//int *s = (int *)aligned_alloc(32,runlength*sizeof(int));


	int ss[runlength];
	presum(a,ss,runlength,1);
	if(argc<2){
		cout<<"loop"<<endl;
		presum(a,s,runlength, runnum);
	}else{
		int vec = atoi(argv[1]);
		if(vec == 128){
			cout<<"128"<<endl;
			presum128(a,s,runlength, runnum);
		}else{
			cout<<"256"<<endl;
			presum256(a,s,runlength, runnum);
		}
	}


	for(int i=0;i<runlength;i++){
		cout<<a[i]<<endl;
	}

}
