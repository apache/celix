/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * hessian_2.0_in.h
 *
 *  \date       Aug 1, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef HESSIAN_2_0_IN_H_
#define HESSIAN_2_0_IN_H_

#include <stdbool.h>

#include "hessian_2.0.h"

typedef struct hessian * hessian_in_pt;

int hessian_readBoolean(hessian_in_pt in, bool *value);
int hessian_readInt(hessian_in_pt in, int *value);
int hessian_readLong(hessian_in_pt in, long *value);
int hessian_readDouble(hessian_in_pt in, double *value);
int hessian_readUTCDate(hessian_in_pt in, long *value);
int hessian_readNull(hessian_in_pt in);
int hessian_readChar(hessian_in_pt in, char *value);
int hessian_readString(hessian_in_pt in, char **value, unsigned int *readLength);
int hessian_readNString(hessian_in_pt in, int offset, int length, char **value, unsigned int *readLength);
int hessian_readByte(hessian_in_pt in, unsigned char *value);
int hessian_readBytes(hessian_in_pt in, unsigned char **value, unsigned int *readLength);
int hessian_readNBytes(hessian_in_pt in, int offset, int length, unsigned char **value, unsigned int *readLength);


#endif /* HESSIAN_2_0_IN_H_ */
