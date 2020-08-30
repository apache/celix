/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * calculator_impl.h
 *
 *  \date       Oct 5, 2011
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef CALCULATOR_IMPL_H_
#define CALCULATOR_IMPL_H_

#include "celix_errno.h"

#include "calculator_service.h"

typedef struct calculator {
} calculator_t;

calculator_t* calculator_create(void);
void calculator_destroy(calculator_t *calculator);
int calculator_add(calculator_t *calculator, double a, double b, double *result);
int calculator_sub(calculator_t *calculator, double a, double b, double *result);
int calculator_sqrt(calculator_t *calculator, double a, double *result);

#endif /* CALCULATOR_IMPL_H_ */
