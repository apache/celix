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
 * resource_processor.h
 *
 *  \date       Feb 13, 2012
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef RESOURCE_PROCESSOR_H_
#define RESOURCE_PROCESSOR_H_

#include "celix_errno.h"

#define DEPLOYMENTADMIN_RESOURCE_PROCESSOR_SERVICE "resource_processor"

typedef struct resource_processor *resource_processor_pt;

typedef struct resource_processor_service *resource_processor_service_pt;

struct resource_processor_service {
	resource_processor_pt processor;
	celix_status_t (*begin)(resource_processor_pt processor, char *packageName);

	celix_status_t (*process)(resource_processor_pt processor, char *name, char *path);

	celix_status_t (*dropped)(resource_processor_pt processor, char *name);
	celix_status_t (*dropAllResources)(resource_processor_pt processor);

	//celix_status_t (*prepare)(resource_processor_pt processor);
	//celix_status_t (*commit)(resource_processor_pt processor);
	//celix_status_t (*rollback)(resource_processor_pt processor);

	//celix_status_t (*cancel)(resource_processor_pt processor);
};

#endif /* RESOURCE_PROCESSOR_H_ */
