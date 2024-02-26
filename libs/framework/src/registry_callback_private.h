/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * service_reference_private.h
 *
 *  \date       Nov 16, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef REGISTRY_CALLBACK_H_
#define REGISTRY_CALLBACK_H_

#include "celix_errno.h"
#include "celix_types.h"
#include "service_reference.h"
#include "service_registration.h"
#include <stdbool.h>

typedef struct registry_callback_struct {
	void *handle;
    celix_status_t (*getUsingBundles)(void *handle, service_registration_pt reg, celix_array_list_t** bundles);
	celix_status_t (*unregister)(void *handle, bundle_pt bundle, service_registration_pt reg);
    bool (*tryRemoveServiceReference)(void *handle, service_reference_pt ref);
} registry_callback_t;

#endif /* REGISTRY_CALLBACK_H_ */
