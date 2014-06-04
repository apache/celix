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
 * hessian_2.0.h
 *
 *  \date       Jul 31, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef HESSIAN_2_0_OUT_H_
#define HESSIAN_2_0_OUT_H_

#include <stdbool.h>

//#include "linked_list.h"
//#include "array_list.h"
#include "hessian_2.0.h"

typedef struct hessian * hessian_out_pt;

int hessian_writeBoolean(hessian_out_pt out, bool value);
int hessian_writeInt(hessian_out_pt out, int value);
int hessian_writeLong(hessian_out_pt out, long value);
int hessian_writeDouble(hessian_out_pt out, double value);
int hessian_writeUTCDate(hessian_out_pt out, long value);
int hessian_writeNull(hessian_out_pt out);
int hessian_writeString(hessian_out_pt out, char *value);
int hessian_writeNString(hessian_out_pt out, char *value, int offset, int length);
int hessian_writeBytes(hessian_out_pt out, unsigned char value[], int length);
int hessian_writeNBytes(hessian_out_pt out, unsigned char value[], int offset, int length);

int hessian_writeListBegin(hessian_out_pt out, int size, char *type);
int hessian_writeListEnd(hessian_out_pt out);

int hessian_writeObjectBegin(hessian_out_pt out, char *type);
int hessian_writeObjectDefinition(hessian_out_pt out, char **fieldNames, int fields);
int hessian_writeObjectEnd(hessian_out_pt out);

int hessian_startCall(hessian_out_pt out, char *method, int length);
int hessian_completeCall(hessian_out_pt out);



#endif /* HESSIAN_2_0_OUT_H_ */
