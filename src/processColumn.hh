/*
 * processColumn.hh
 *
 *  Created on: Jun 29, 2016
 *      Author: teng
 */

#ifndef SRC_PROCESSCOLUMN_HH_
#define SRC_PROCESSCOLUMN_HH_
namespace orc{
	struct ColumnPattern{

		uint64_t repeated = 0;
		uint64_t bitSize = 0;
		uint64_t runLength = 0;

		ColumnPattern(uint64_t repeated = 0, uint64_t bitSize = 0, uint64_t runLength = 0);
		ColumnPattern(const ColumnPattern &pattern);


	};

	std::vector<ColumnPattern> readPattern(char *data, uint64_t datasize);

}

#endif /* SRC_PROCESSCOLUMN_HH_ */
