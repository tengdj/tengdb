/*
 * codegen_function.hh
 *
 *  Created on: Jun 28, 2016
 *      Author: teng
 */

#ifndef INCLUDE_CODEGEN_FUNCTION_HH_
#define INCLUDE_CODEGEN_FUNCTION_HH_

#include <llvm/IR/Function.h>
#include "processColumn.hh"
namespace orc{

llvm::Function* genfunc_specific(LlvmCodeGen *gen);

}



#endif /* INCLUDE_CODEGEN_FUNCTION_HH_ */
