/*
 * SQLCodegen.cpp
 *
 *  Created on: Nov 22, 2016
 *      Author: teng
 */
#include "llvm-codegen.h"
#include <map>
#include "sql/Expr.h"
#include <iostream>
#include "Node.h"
#include "config.h"
#include "util.h"
#include "Scanner.h"
#include "Table.h"
#include "sql/SelectStatement.h"

using namespace llvm;
using namespace orc;
namespace hsql{


Node *SelectStatement::codegen(){

	this->groupBy;
	this->fromTable;
	this->selectList;
	this->whereClause;

	vector<Column> columnlist = this->selectedFields();

	Table *table = getTable(this->fromTable->getName());

	Node *node = new ScanNode(table,columnlist);
	reinterpret_cast<ScanNode *>(node)->prepare();

	return node;

}


}


