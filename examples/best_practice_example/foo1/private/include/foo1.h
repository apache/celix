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

#ifndef FOO1_H_
#define FOO1_H_

#include "example.h"

typedef struct foo1_struct foo1_t;

foo1_t* foo1_create(void);
void foo1_destroy(foo1_t *self);

int foo1_start(foo1_t *self);
int foo1_stop(foo1_t *self);

int foo1_setExample(foo1_t *self, const example_t *example);


#endif //FOO1_H_
