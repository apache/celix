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
 * service_metadata.h
 *
 *  \date       Nov 3, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#ifndef SERVICE_METADATA_H_
#define SERVICE_METADATA_H_

#include <stdbool.h>
#include <apr_general.h>

#include <celix_errno.h>

typedef struct service *service_t;

celix_status_t service_create(apr_pool_t *pool, service_t *component);

celix_status_t serviceMetadata_setServiceFactory(service_t currentService, bool serviceFactory);
celix_status_t serviceMetadata_isServiceFactory(service_t currentService, bool *serviceFactory);

celix_status_t serviceMetadata_addProvide(service_t service, char *provide);
celix_status_t serviceMetadata_getProvides(service_t service, char **provides[], int *size);

#endif /* SERVICE_METADATA_H_ */
