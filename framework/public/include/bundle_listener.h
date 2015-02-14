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
 * @defgroup BundleListener Bundle Listener
 * @ingroup framework
 * @{
 *
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \date      	June 28, 2012
 *  \copyright	Apache License, Version 2.0
 */
#ifndef BUNDLE_LISTENER_H_
#define BUNDLE_LISTENER_H_

typedef struct bundle_listener *bundle_listener_pt;

#include "celix_errno.h"
#include "bundle_event.h"

struct bundle_listener {
	void * handle;
	celix_status_t (*bundleChanged)(void * listener, bundle_event_pt event);
};



#endif /* service_listener_t_H_ */

/**
 * @}
 */
