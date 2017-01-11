/*
 * Node.h
 *
 *  Created on: Nov 9, 2016
 *      Author: teng
 */

#ifndef INCLUDE_NODE_H_
#define INCLUDE_NODE_H_
#include "util.h"
#include "llvm-codegen.h"
#include "functions-ir.h"
#include "Scanner.h"
#include <string>
#include "HashTable.h"


namespace tengdb{

class Node{

public:
	void prepare(LlvmCodeGen *codegen);
	Batch* nextBatch();
	~Node(){
	}
};


class ScanNode:public Node{

private:
	Table *table;
	std::vector<Column> columns;
	Scanner *scanner;

public:
	ScanNode(std::string tablename, std::vector<Column> columns);
	ScanNode(Table *table, std::vector<Column> columns);

	void prepare();
	Batch* nextBatch();
	~ScanNode();

};

class AggregateNode:public Node{

private:
	Node *childNode;
	Table *table;
	HashTable *hashTable;
public:
	AggregateNode(std::vector<Column> groupon);
	void prepare();
	Batch* nextBatch();

};

}

#endif /* INCLUDE_NODE_H_ */
