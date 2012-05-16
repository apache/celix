/*
 * hessian_2.0_in.h
 *
 *  Created on: Aug 1, 2011
 *      Author: alexander
 */

#ifndef HESSIAN_2_0_IN_H_
#define HESSIAN_2_0_IN_H_

#include <stdbool.h>

#include "hessian_2.0.h"

typedef struct hessian * hessian_in_t;

int hessian_readBoolean(hessian_in_t in, bool *value);
int hessian_readInt(hessian_in_t in, int *value);
int hessian_readLong(hessian_in_t in, long *value);
int hessian_readDouble(hessian_in_t in, double *value);
int hessian_readUTCDate(hessian_in_t in, long *value);
int hessian_readNull(hessian_in_t in);
int hessian_readChar(hessian_in_t in, char *value);
int hessian_readString(hessian_in_t in, char **value, unsigned int *readLength);
int hessian_readNString(hessian_in_t in, int offset, int length, char **value, unsigned int *readLength);
int hessian_readByte(hessian_in_t in, unsigned char *value);
int hessian_readBytes(hessian_in_t in, unsigned char **value, unsigned int *readLength);
int hessian_readNBytes(hessian_in_t in, int offset, int length, unsigned char **value, unsigned int *readLength);


#endif /* HESSIAN_2_0_IN_H_ */
