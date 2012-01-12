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
 * attribute.h
 *
 *  Created on: Jul 27, 2010
 *      Author: alexanderb
 */

#ifndef ATTRIBUTE_H_
#define ATTRIBUTE_H_

#include <apr_general.h>
#include "celix_errno.h"

typedef struct attribute * ATTRIBUTE;

celix_status_t attribute_create(char * key, char * value, apr_pool_t *memory_pool, ATTRIBUTE *attribute);

celix_status_t attribute_getKey(ATTRIBUTE attribute, char **key);
celix_status_t attribute_getValue(ATTRIBUTE attribute, char **value);

#endif /* ATTRIBUTE_H_ */
