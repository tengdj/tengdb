/*
 * TestScanNode.cpp
 *
 *  Created on: Nov 12, 2016
 *      Author: teng
 */

#include "Node.h"
#include <thread>
#include <iostream>
#include "RLE2.h"
#include "Scanner.h"
#include "Table.h"

using namespace orc;
using namespace std;
//
//sum(l_quantity),
//      sum(l_extendedprice),
//      sum(l_extendedprice * (1 - l_discount)),
//      sum(l_extendedprice * (1 - l_discount) * (1 + l_tax)),
//      avg(l_discount),


std::mutex mtx;
LlvmCodeGen *gen;
Table *lineitem;
std::vector<Column> columns;

double global[6][5];
int globalcount[6];

void clearresult(double **data){

	for(int i=0;i<6;i++){
		for(int j=0;j<5;j++){
			data[i][j] = 0;
		}
	}
}

void update(Batch *batch, double **local, int *localcount){


	double *quantity = (double *)batch->data["l_quantity"]->data();
	double *price = (double *)batch->data["l_extendedprice"]->data();
	double *discount = (double *)batch->data["l_discount"]->data();
	double *tax = (double *)batch->data["l_tax"]->data();
	uint32_t *returnflag = (uint32_t *)batch->data["l_returnflag"]->data();
	uint32_t *linestatus = (uint32_t *)batch->data["l_linestatus"]->data();


	uint32_t hash;
	for(uint64_t i=0;i<batch->rownumber;i++){
		hash = returnflag[i]*2+linestatus[i];
		hash = linestatus[i];
		hash = returnflag[i];
		hash = returnflag[i]*2+linestatus[i];

		local[hash][0] += price[i];
		local[hash][1] += price[i]*(1-discount[i]);
		local[hash][2] += price[i]*(1-discount[i])*(1+tax[i]);
		local[hash][3] += quantity[i];
		local[hash][4] += discount[i];
		localcount[hash]++;
	}


}

void call_thread(int curid){

	double *local[6];
	int localcount[256];
	local[0] = new double[5];
	local[1] = new double[5];
	local[2] = new double[5];
	local[3] = new double[5];
	local[4] = new double[5];
	local[5] = new double[5];
	clearresult(local);
	for(int i=0;i<6;i++){
		localcount[i] = 0;
	}
	ORCScanner *scanner = new ORCScanner(lineitem,columns);
	scanner->prepare();
	while(true){
		Batch *batch = scanner->nextBatch();

		if(batch->eof){
			break;
		}
		update(batch,local,localcount);
		//batch->print();
	}


	mtx.lock();
	for(int i=0;i<6;i++){
		for(int k=0;k<5;k++){
			global[i][k] += local[i][k];
		}
		globalcount[i] += localcount[i];
	}
	mtx.unlock();
	delete scanner;
}

int main(int argc, char **argv){


	std::string tablename = "lineitem";
	for(int i=0;i<6;i++){
		for(int j=0;j<5;j++){
			global[i][j] = 0;
		}
	}
	for(int i=0;i<6;i++){
		globalcount[i] = 0;
	}
	int threads_num = sysconf(_SC_NPROCESSORS_ONLN)+2;;
	for (int i = 1; i < argc; i++) {
		double d;
		int n;
		int n2;
		long long ll;
		int64_t n64;
		char junk;
		if (sscanf(argv[i], "-t=%d%c", &n, &junk) == 1) {
			threads_num = n;
		} else if(sscanf(argv[i], "-g=%d%c", &n, &junk) == 1) {
			orc::FLAGS_codegen = n;
		} else {
			fprintf(stderr, "Invalid flag '%s'\n", argv[i]);
			exit(1);
		}
	}
	lineitem = orc::getTable(tablename);

	columns.push_back(Column("l_tax",lineitem->getTypeKind("l_tax")));
	columns.push_back(Column("l_extendedprice",lineitem->getTypeKind("l_extendedprice")));
	columns.push_back(Column("l_quantity",lineitem->getTypeKind("l_quantity")));
	columns.push_back(Column("l_discount",lineitem->getTypeKind("l_discount")));
	columns.push_back(Column("l_returnflag",lineitem->getTypeKind("l_returnflag")));
	columns.push_back(Column("l_linestatus",lineitem->getTypeKind("l_linestatus")));
	//columns.push_back(Column("l_partkey",lineitem->getTypeKind("l_partkey")));

	std::thread t[threads_num];
	for(int i=0;i<threads_num;i++){
		t[i] = std::thread(call_thread,i);
	}
	cout<<threads_num<<" threads are started"<<endl;
	for(int i=0;i<threads_num;i++){
		t[i].join();
	}
	for(int i=0;i<6;i++){
		cout<<i<<"\t"<<globalcount[i]<<"\t";
		for(int j=0;j<5;j++){
			cout<<"\t"<<global[i][j];
		}
		cout<<endl;
	}

	delete lineitem;




}



