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
 * endpoint_listener.h
 *
 *  \date       Sep 29, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef ENDPOINT_LISTENER_H_
#define ENDPOINT_LISTENER_H_

#include "array_list.h"
#include "properties.h"

#include "endpoint_description.h"

static const char * const OSGI_ENDPOINT_LISTENER_SERVICE = "endpoint_listener";

static const char * const OSGI_ENDPOINT_LISTENER_SCOPE = "endpoint.listener.scope";

struct endpoint_listener {
	void *handle;
	celix_status_t (*endpointAdded)(void *handle, endpoint_description_pt endpoint, char *machtedFilter);
	celix_status_t (*endpointRemoved)(void *handle, endpoint_description_pt endpoint, char *machtedFilter);
};

typedef struct endpoint_listener *endpoint_listener_pt;


#endif /* ENDPOINT_LISTENER_H_ */
