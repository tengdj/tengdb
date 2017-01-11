/*
 * config.cpp
 *
 *  Created on: Jun 27, 2016
 *      Author: teng
 */

#include "../include/config.h"
#include <thread>
#include <unistd.h>

namespace orc{

	int type = 1;
	std::string irpath = "./";
	std::string dbpath = "/home/teng/orcfile/";
	uint64_t processAddr = 0;
	int FLAGS_threads_number = sysconf(_SC_NPROCESSORS_ONLN)+2;
	bool FLAGS_codegen = true;
	bool FLAGS_vectorization = false;
	int FLAGS_fetch_top = 5;
	bool FLAGS_use_optimized_module = true;
	int FLAGS_optimization_level = 3;
	void parseGlobalFlag(int argc, char **argv){
		for (int i = 1; i < argc; i++) {
				double d;
				int n;
				int n2;
				long long ll;
				int64_t n64;
				char junk;

				if (sscanf(argv[i], "-t=%d%c", &n, &junk) == 1) {
					orc::FLAGS_threads_number = n;
				} else if (strncmp(argv[i], "-v",2) == 0) {
					orc::FLAGS_vectorization = true;
				} else if(sscanf(argv[i], "-o=%d%c", &n, &junk) == 1) {
					orc::FLAGS_use_optimized_module = n;
				} else if(sscanf(argv[i], "-O%d%c", &n, &junk) == 1) {
					orc::FLAGS_optimization_level = n;
				} else if(sscanf(argv[i], "-g=%d%c", &n, &junk) == 1) {
					orc::FLAGS_codegen = n;
				} else if (sscanf(argv[i], "-d=%d%c", &n, &junk) == 1) {
					orc::double_per_round = n;
				} else if (sscanf(argv[i], "-ft=%d%c", &n, &junk) == 1) {
					orc::FLAGS_fetch_top = n;
				}
		}
	}


}
