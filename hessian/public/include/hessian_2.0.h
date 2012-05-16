/*
 * hessian_2.0.h
 *
 *  Created on: Aug 1, 2011
 *      Author: alexander
 */

#ifndef HESSIAN_2_0_H_
#define HESSIAN_2_0_H_

#include "hessian_2.0_in.h"
#include "hessian_2.0_out.h"

#include "hessian_constants.h"

struct hessian {
	long offset;
	long length;
	long capacity;

	unsigned char *buffer;

	bool lastChunk;
	unsigned int chunkLength;
};

#endif /* HESSIAN_2_0_H_ */
