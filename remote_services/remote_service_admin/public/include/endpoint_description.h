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
 * endpoint_description.h
 *
 *  \date       25 Jul 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef ENDPOINT_DESCRIPTION_H_
#define ENDPOINT_DESCRIPTION_H_

#include "properties.h"
#include "array_list.h"

struct endpoint_description {
    char *frameworkUUID;
    char *id;
    // array_list_pt intents;
    char *service;
    // HASH_MAP packageVersions;
    properties_pt properties;
    long serviceId;
};

typedef struct endpoint_description *endpoint_description_pt;

celix_status_t endpointDescription_create(properties_pt properties, endpoint_description_pt *endpointDescription);
celix_status_t endpointDescription_destroy(endpoint_description_pt description);


#endif /* ENDPOINT_DESCRIPTION_H_ */
