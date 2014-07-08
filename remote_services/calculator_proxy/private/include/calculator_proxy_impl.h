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
 * calculator_proxy_impl.h
 *
 *  \date       Oct 13, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CALCULATOR_PROXY_IMPL_H_
#define CALCULATOR_PROXY_IMPL_H_

#include <apr_general.h>

#include "celix_errno.h"

#include "calculator_service.h"
#include "remote_proxy.h"
#include "constants.h"
#include "bundle_context.h"
#include "hash_map.h"

#include "endpoint_listener.h"


struct calculator {
	apr_pool_t *pool;
	bundle_context_pt context;

	endpoint_description_pt endpoint;
	sendToHandle sendToCallback;
	void * sendToHandler;
};


celix_status_t calculatorProxy_create(apr_pool_t *pool, bundle_context_pt context, calculator_pt *endpoint);
celix_status_t calculatorProxy_add(calculator_pt calculator, double a, double b, double *result);
celix_status_t calculatorProxy_sub(calculator_pt calculator, double a, double b, double *result);
celix_status_t calculatorProxy_sqrt(calculator_pt calculator, double a, double *result);

celix_status_t calculatorProxy_registerProxyService(void* proxyFactoryService, endpoint_description_pt endpoint, void* handler, sendToHandle callback);
celix_status_t calculatorProxy_unregisterProxyService(void* proxyFactoryService, endpoint_description_pt endpoint);

#endif /* CALCULATOR_PROXY_IMPL_H_ */
