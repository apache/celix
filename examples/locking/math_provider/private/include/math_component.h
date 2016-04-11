/*
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
#ifndef MATH_COMPONENT_H_
#define MATH_COMPONENT_H_

#include "celix_errno.h"

typedef struct math_component *math_component_pt;

celix_status_t mathComponent_create(math_component_pt *math);
celix_status_t mathComponent_destroy(math_component_pt math);

int mathComponent_calc(math_component_pt math, int arg1, int arg2);


#endif /* MATH_COMPONENT_H_ */
