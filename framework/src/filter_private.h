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
 * filter_private.h
 *
 *  \date       Feb 13, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#ifndef FILTER_PRIVATE_H_
#define FILTER_PRIVATE_H_

#include "filter.h"
#include "array_list.h"

typedef enum operand
{
	EQUAL,
	APPROX,
	GREATER,
    GREATEREQUAL,
	LESS,
	LESSEQUAL,
	PRESENT,
	SUBSTRING,
	AND,
	OR,
	NOT,
} OPERAND;

struct filter {
	OPERAND operand;
	char * attribute;
	void * value;
	char *filterStr;
};


#endif /* FILTER_PRIVATE_H_ */
