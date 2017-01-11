/*
 * ScanNode.cpp
 *
 *  Created on: Nov 9, 2016
 *      Author: teng
 */

#include "Node.h"
#include "functions-ir.h"
#include "Scanner.h"


namespace tengdb{

ScanNode::ScanNode(Table *table,std::vector<Column> columns){

	this->columns = columns;
	this->table = table;
	this->scanner = new ORCScanner(table, columns);
}

ScanNode::ScanNode(std::string tablename, std::vector<Column> columns){

	Table *table = getTable(tablename);
	this->columns = columns;
	this->table = table;
	this->scanner = new ORCScanner(table, columns);

}

void ScanNode::prepare(){
	reinterpret_cast<ORCScanner *>(scanner)->prepare();
}

Batch * ScanNode::nextBatch(){
	return reinterpret_cast<ORCScanner *>(scanner)->nextBatch();

}

ScanNode::~ScanNode(){
	if(scanner){
		delete reinterpret_cast<ORCScanner *>(scanner);
	}
	if(table){
		delete table;
	}
}

}


