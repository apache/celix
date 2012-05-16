/*
 * hessian_2.0.h
 *
 *  Created on: Jul 31, 2011
 *      Author: alexander
 */

#ifndef HESSIAN_2_0_OUT_H_
#define HESSIAN_2_0_OUT_H_

#include <stdbool.h>

//#include "linkedlist.h"
//#include "array_list.h"
#include "hessian_2.0.h"

typedef struct hessian * hessian_out_t;

int hessian_writeBoolean(hessian_out_t out, bool value);
int hessian_writeInt(hessian_out_t out, int value);
int hessian_writeLong(hessian_out_t out, long value);
int hessian_writeDouble(hessian_out_t out, double value);
int hessian_writeUTCDate(hessian_out_t out, long value);
int hessian_writeNull(hessian_out_t out);
int hessian_writeString(hessian_out_t out, char *value);
int hessian_writeNString(hessian_out_t out, char *value, int offset, int length);
int hessian_writeBytes(hessian_out_t out, unsigned char value[], int length);
int hessian_writeNBytes(hessian_out_t out, unsigned char value[], int offset, int length);

int hessian_writeListBegin(hessian_out_t out, int size, char *type);
int hessian_writeListEnd(hessian_out_t out);

int hessian_writeObjectBegin(hessian_out_t out, char *type);
int hessian_writeObjectDefinition(hessian_out_t out, char **fieldNames, int fields);
int hessian_writeObjectEnd(hessian_out_t out);

int hessian_startCall(hessian_out_t out, char *method, int length);
int hessian_completeCall(hessian_out_t out);



#endif /* HESSIAN_2_0_OUT_H_ */
