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
 * capability.h
 *
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CAPABILITY_H_
#define CAPABILITY_H_

typedef struct capability *capability_pt;

#include "hash_map.h"
#include "module.h"

celix_status_t capability_create(module_pt module, hash_map_pt directives, hash_map_pt attributes, capability_pt *capability);
celix_status_t capability_destroy(capability_pt capability);
celix_status_t capability_getServiceName(capability_pt capability, char **serviceName);
celix_status_t capability_getVersion(capability_pt capability, version_pt *version);
celix_status_t capability_getModule(capability_pt capability, module_pt *module);

#endif /* CAPABILITY_H_ */
