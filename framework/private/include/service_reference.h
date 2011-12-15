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
 * service_reference.h
 *
 *  Created on: Jul 20, 2010
 *      Author: alexanderb
 */

#ifndef SERVICE_REFERENCE_H_
#define SERVICE_REFERENCE_H_

#include "celixbool.h"
#include "headers.h"

celix_status_t serviceReference_create(apr_pool_t *pool, BUNDLE bundle, SERVICE_REGISTRATION registration, SERVICE_REFERENCE *reference);

celix_status_t serviceReference_invalidate(SERVICE_REFERENCE reference);

celix_status_t serviceReference_getServiceRegistration(SERVICE_REFERENCE reference, SERVICE_REGISTRATION *registration);
celix_status_t serviceReference_getBundle(SERVICE_REFERENCE reference, BUNDLE *bundle);

bool serviceReference_isAssignableTo(SERVICE_REFERENCE reference, BUNDLE requester, char * serviceName);

celix_status_t serviceReference_getUsingBundles(SERVICE_REFERENCE reference, apr_pool_t *pool, ARRAY_LIST *bundles);
celix_status_t serviceReference_equals(SERVICE_REFERENCE reference, SERVICE_REFERENCE compareTo, bool *equal);

#endif /* SERVICE_REFERENCE_H_ */
