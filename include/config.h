/*
 * config.hh
 *
 *  Created on: Jun 27, 2016
 *      Author: teng
 */

#ifndef INCLUDE_CONFIG_H_
#define INCLUDE_CONFIG_H_

#include <cstring>
#include <stdlib.h>
#include <fstream>

using namespace std;
namespace tengdb{

extern int type;
extern std::string irpath;
extern uint64_t processAddr;
extern int FLAGS_threads_number;
extern std::string dbpath;
extern bool FLAGS_codegen;
extern bool FLAGS_vectorization;
static uint64_t double_per_round = 500;
extern int FLAGS_fetch_top;
extern bool FLAGS_use_optimized_module;
extern int FLAGS_optimization_level;
extern bool FLAGS_print_batch;

void parseGlobalFlag(int argc, char **argv);


}



#endif /* INCLUDE_CONFIG_H_ */
