#ifndef __SELECT_STATEMENT_H__
#define __SELECT_STATEMENT_H__

#include "../../frontend/sql/Expr.h"
#include "../../frontend/sql/SQLStatement.h"
#include "../../frontend/sql/Table.h"
#include "llvm-codegen.h"
#include "Scanner.h"
#include "Node.h"
#include "Table.h"


using namespace llvm;
using namespace tengdb;

namespace hsql {
    typedef enum {
        kOrderAsc,
        kOrderDesc
    } OrderType;

    /**
     * Description of the order by clause within a select statement
     * TODO: hold multiple expressions to be sorted by
     */
    struct OrderDescription {
        OrderDescription(OrderType type, Expr* expr) :
            type(type),
            expr(expr) {}

        virtual ~OrderDescription() {
            delete expr;
        }

        OrderType type;
        Expr* expr;
    };

    const int64_t kNoLimit = -1;
    const int64_t kNoOffset = -1;

    /**
     * Description of the limit clause within a select statement
     */
    struct LimitDescription {
        LimitDescription(int64_t limit, int64_t offset) :
            limit(limit),
            offset(offset) {}

        int64_t limit;
        int64_t offset;
    };

    /**
     * Description of the group-by clause within a select statement
     */
    struct GroupByDescription {
        GroupByDescription() :
            columns(NULL),
            having(NULL) {}

        ~GroupByDescription() {
            delete columns;
            delete having;
        }

        std::vector<Expr*>* columns;
        Expr* having;
    };

    /**
     * Representation of a full SQL select statement.
     * TODO: add union_order and union_limit
     */
    struct SelectStatement : SQLStatement {
        SelectStatement() :
            SQLStatement(kStmtSelect),
            fromTable(NULL),
            selectDistinct(false),
            selectList(NULL),
            whereClause(NULL),
            groupBy(NULL),
            unionSelect(NULL),
            order(NULL),
            limit(NULL) {};

        virtual ~SelectStatement() {
            delete fromTable;
            delete selectList;
            delete whereClause;
            delete groupBy;
            delete order;
            delete limit;
        }

        Node *codegen();

        std::vector<Column> selectedFields(){
			std::map<std::string,bool> columns;
			if(this->groupBy){
				for(Expr *expr:*this->groupBy->columns){
					expr->addColumn(&columns);
				}
			}
			if(this->selectList){
				for(Expr *expr:*this->selectList){
					expr->addColumn(&columns);
				}
			}
			Table *table = getTable(std::string(this->fromTable->getName()));
			std::vector<Column> columnlist;
			for(std::map<std::string, bool>::iterator i = columns.begin();i!=columns.end();i++){
				columnlist.push_back(Column(i->first,table->getTypeKind(i->first)));
			}
			return columnlist;
		}

        TableRef* fromTable;
        bool selectDistinct;
        std::vector<Expr*>* selectList;
        Expr* whereClause;
        GroupByDescription* groupBy;

        SelectStatement* unionSelect;
        OrderDescription* order;
        LimitDescription* limit;
    };

} // namespace hsql
#endif
