
#include "../../frontend/sql/SelectStatement.h"
#include "../../frontend/sql/Table.h"

namespace hsql {


    TableRef::~TableRef() {
        delete name;
        delete alias;
        delete select;
        delete list;
    }


} // namespace hsql