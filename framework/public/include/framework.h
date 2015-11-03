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
 * framework.h
 *
 *  \date       Mar 23, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef FRAMEWORK_H_
#define FRAMEWORK_H_

typedef struct activator * activator_pt;
typedef struct framework * framework_pt;

#include "celix_errno.h"
#include "framework_exports.h"
#include "bundle.h"
#include "properties.h"

// #TODO: Move to FrameworkFactory according the OSGi Spec
FRAMEWORK_EXPORT celix_status_t framework_create(framework_pt *framework, properties_pt config);
FRAMEWORK_EXPORT celix_status_t framework_destroy(framework_pt framework);

FRAMEWORK_EXPORT celix_status_t fw_init(framework_pt framework);
FRAMEWORK_EXPORT celix_status_t framework_waitForStop(framework_pt framework);

FRAMEWORK_EXPORT celix_status_t framework_getFrameworkBundle(framework_pt framework, bundle_pt *bundle);

#endif /* FRAMEWORK_H_ */
