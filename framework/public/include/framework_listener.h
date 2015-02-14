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
 * @defgroup FrameworkListener Framework Listener
 * @ingroup framework
 * @{
 *
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \date      	Oct 8, 2013
 *  \copyright	Apache License, Version 2.0
 */
#ifndef FRAMEWORK_LISTENER_H_
#define FRAMEWORK_LISTENER_H_

typedef struct framework_listener *framework_listener_pt;

#include "celix_errno.h"
#include "framework_event.h"

struct framework_listener {
	void * handle;
	celix_status_t (*frameworkEvent)(void * listener, framework_event_pt event);
};



#endif /* FRAMEWORK_LISTENER_H_ */

/**
 * @}
 */
