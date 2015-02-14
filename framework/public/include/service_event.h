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
/**
 *
 * @defgroup ServiceListener Service Listener
 * @ingroup framework
 * @{
 *
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \date      	January 11, 2012
 *  \copyright	Apache License, Version 2.0
 */
#ifndef SERVICE_EVENT_H_
#define SERVICE_EVENT_H_

enum serviceEventType
{
	OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED = 0x00000001,
	OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED = 0x00000002,
	OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING = 0x00000004,
	OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED_ENDMATCH = 0x00000008,
};

typedef enum serviceEventType service_event_type_e;

typedef struct serviceEvent *service_event_pt;

#include "service_reference.h"

#include "service_reference.h"

struct serviceEvent {
	service_reference_pt reference;
	service_event_type_e type;
};

#endif /* SERVICE_EVENT_H_ */

/**
 * @}
 */
