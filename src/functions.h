/*
 * functions.h
 *
 *  Created on: Jul 25, 2016
 *      Author: teng
 */

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <memory>

namespace orc{


void readDouble(unsigned char *data, double *result, uint64_t &index, uint64_t offset, uint64_t length);


}


#endif /* FUNCTIONS_H_ */
