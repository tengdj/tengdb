// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.


#include "util.h"
#include <iostream>

using namespace std;

using namespace tengdb;

int main(int argc, char **argv){

	char bytes[100];
	int value[]{-10,20,32,-100,256};
	int out[5];

	tengdb::EncodeArray(bytes,value,5);
	tengdb::DecodeArray(bytes,out,5);

	for(int i=0;i<5;i++){
		cout<<out[i]<<endl;
	}

	tengdb::EncodeString(bytes,std::string("terry is good"));
	std::string str;
	tengdb::DecodeString(bytes,str);
	cout<<str<<endl;


	return 0;



}
