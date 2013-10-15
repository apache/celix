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
 * calculator_impl.h
 *
 *  \date       Oct 5, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CALCULATOR_IMPL_H_
#define CALCULATOR_IMPL_H_

#include <apr_general.h>

#include "celix_errno.h"

#include "calculator_service.h"

struct calculator {
	apr_pool_t *pool;
};

celix_status_t calculator_create(apr_pool_t *pool, calculator_pt *calcuator);
celix_status_t calculator_add(calculator_pt calcuator, double a, double b, double *result);
celix_status_t calculator_sub(calculator_pt calcuator, double a, double b, double *result);
celix_status_t calculator_sqrt(calculator_pt calcuator, double a, double *result);

#endif /* CALCULATOR_IMPL_H_ */
