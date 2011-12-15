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
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */

#ifndef CAPABILITY_H_
#define CAPABILITY_H_

#include "hash_map.h"
#include "module.h"


celix_status_t capability_create(apr_pool_t *pool, MODULE module, HASH_MAP directives, HASH_MAP attributes, CAPABILITY *capability);
celix_status_t capability_getServiceName(CAPABILITY capability, char **serviceName);
celix_status_t capability_getVersion(CAPABILITY capability, VERSION *version);
celix_status_t capability_getModule(CAPABILITY capability, MODULE *module);

#endif /* CAPABILITY_H_ */
