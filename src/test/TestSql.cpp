
#include <stdlib.h>
#include <string>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

// include the sql parser
#include "SQLParser.h"

// contains printing utilities
#include "sqlhelper.h"

#include "llvm-codegen.h"
#include "Node.h"
#include "../frontend/sql/statements.h"
#include <fstream>

using namespace hsql;
using namespace tengdb;


int main(int argc, char *argv[]) {

	std::string query;
	std::string filepath;
	bool fromfile = false;
	bool printbatch = false;
	parseGlobalFlag(argc,argv);
	for (int i = 1; i < argc; i++) {
		double d;
		int n;
		int n2;
		long long ll;
		int64_t n64;
		char junk;

		if (strncmp(argv[i], "-q=",3) == 0) {
			query = argv[i]+3;
		} else if (strncmp(argv[i], "-f=",3) == 0) {
			filepath = argv[i]+3;
			fromfile = true;
		} else if (strncmp(argv[i], "-p",2) == 0) {
			printbatch = true;
		}
	}

	if(fromfile){
		 std::ifstream ifs(filepath);
		 std::string content( (std::istreambuf_iterator<char>(ifs) ),
		                         (std::istreambuf_iterator<char>()    ) );
		 query = content.c_str();
	}

    // parse a given query
    hsql::SQLParserResult* result = hsql::SQLParser::parseSQLString(query);

    // check whether the parsing was successful
    if (result->isValid) {
        for (hsql::SQLStatement* stmt : result->statements) {
			if(stmt->type()==hsql::StatementType::kStmtSelect){
				SelectStatement *selectstmt = reinterpret_cast<SelectStatement *>(stmt);
				ScanNode *node = reinterpret_cast<ScanNode *>(selectstmt->codegen());
				Batch *batch;
				do{
					batch = node->nextBatch();
					if(batch->eof){
						break;
					}
					if(printbatch){
						batch->print();
					}
				}while(true);
				delete node;
            }
        }

        return 0;
    } else {
        printf("Invalid SQL!\n");
        return -1;
    }
}
