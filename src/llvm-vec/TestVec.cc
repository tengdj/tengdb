/*
 * TestVec.cpp
 *
 *  Created on: Dec 8, 2016
 *      Author: teng
 */

#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
using namespace std;

void shuffle128(unsigned int *result, unsigned char *byte);
void shuffle256(unsigned int *result, unsigned char *byte);


int main(int argc, char **argv){

	unsigned char byte[32];
	byte[0]  = (unsigned char)20;
	byte[1]  = (unsigned char)113;
	byte[2]  = (unsigned char)250;
	byte[3]  = (unsigned char)10;
	byte[4]  = (unsigned char)195;
	byte[5]  = (unsigned char)172;
	byte[6]  = (unsigned char)7;
	byte[7]  = (unsigned char)161;
	byte[8]  = (unsigned char)40;
	byte[9]  = (unsigned char)55;
	byte[10] = (unsigned char)25;
	byte[11] = (unsigned char)72;
	byte[12] = (unsigned char)19;
	byte[13] = (unsigned char)1;
	byte[14] = (unsigned char)14;
	byte[15] = (unsigned char)1;
	byte[16] = (unsigned char)36;
	byte[17] = (unsigned char)154;
	byte[18] = (unsigned char)44;
	byte[19] = (unsigned char)173;
	byte[20] = (unsigned char)130;
	byte[21] = (unsigned char)44;
	byte[22] = (unsigned char)164;
	byte[23] = (unsigned char)92;
	byte[24] = (unsigned char)54;
	byte[25] = (unsigned char)25;
	byte[26] = (unsigned char)154;
	byte[27] = (unsigned char)17;
	byte[28] = (unsigned char)233;
	byte[29] = (unsigned char)80;
	byte[30] = (unsigned char)14;
	byte[31] = (unsigned char)1;
	unsigned int result[8];

	shuffle128(result,byte);

	for(int i=0;i<4;i++){
		printf("%d: %d\n",i,result[i]);
	}
	shuffle256(result,byte);

	for(int i=0;i<8;i++){
		printf("%d: %d\n",i,result[i]);
	}

	return 0;
}

