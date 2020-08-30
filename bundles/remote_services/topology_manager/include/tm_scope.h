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
 * tm_scope.h
 *
 *  \date       Oct 29, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef TM_SCOPE_H_
#define TM_SCOPE_H_

#include "celix_errno.h"

#define TOPOLOGYMANAGER_SCOPE_SERVICE "tm_scope"


struct tm_scope_service {
    void *handle;	// scope_pt
    celix_status_t (*addExportScope)(void *handle, char *filter, celix_properties_t *props);
    celix_status_t (*removeExportScope)(void *handle, char *filter);
    celix_status_t (*addImportScope)(void *handle, char *filter);
    celix_status_t (*removeImportScope)(void *handle, char *filter);
};

typedef struct tm_scope_service tm_scope_service_t;
typedef tm_scope_service_t *tm_scope_service_pt;

#endif /* TM_SCOPE_H_ */
