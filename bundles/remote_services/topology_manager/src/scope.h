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
 * scope.h
 *
 *  \date       Sep 29, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef TOPOLOGY_SCOPE_H_
#define TOPOLOGY_SCOPE_H_

#include <stdbool.h>

#include "celix_errno.h"
#include "celix_threads.h"
#include "hash_map.h"
#include "endpoint_description.h"
#include "celix_properties.h"
#include "service_reference.h"
#include "tm_scope.h"

typedef struct scope *scope_pt;



/* \brief  create scope structure
 *
 * \param  owning component pointer
 * \param  scope to be created
 *
 * \return CELIX_SUCCESS
 *         CELIX_ENOMEM
 */
celix_status_t scope_scopeCreate(void *handle, scope_pt *scope);

/* \brief  destroy scope structure
 *
 * \param  scope to be destroyed
 *
 * \return CELIX_SUCCESS
 */
celix_status_t scope_scopeDestroy(scope_pt scope);

/* \brief  register export scope change callback of topology manager
 *
 * \param  scope structure
 * \param  changed function pointer
 *
 * \return -
 */
void scope_setExportScopeChangedCallback(scope_pt scope, celix_status_t (*changed)(void *handle, char *servName));

/* \brief  register import scope change callback of topology manager
 *
 * \param  scope structure
 * \param  changed function pointer
 *
 * \return -
 */
void scope_setImportScopeChangedCallback(scope_pt scope, celix_status_t (*changed)(void *handle, char *servName));


/* \brief  Test if scope allows import of service
 *
 * \param  scope containing import rules
 * \param  endpoint import service endpoint description
 *
 * \return true import allowed
 *         false import not allowed
 */
bool scope_allowImport(scope_pt scope, endpoint_description_t *endpoint);

/* \brief  Test if scope allows import of service
 *
 * \param  scope containing export rules
 * \param  reference to service
 * \param  props, additional properties defining restrictions for the exported service
 *                NULL if no additional restrictions found
 *
 * \return CELIX_SUCCESS
 *
 */
celix_status_t scope_getExportProperties(scope_pt scope, service_reference_pt reference, celix_properties_t **props);

/* \brief  add restricted scope for specified exported service
 *
 * \param  handle pointer to scope
 * \param  filter, filter string
 * \param  props additional properties defining restrictions for the exported service
 *
 * \return CELIX_SUCCESS if added to scope
 *         CELIX_ILLEGAL_ARGUMENT if service scope is already restricted before
 *
 */
celix_status_t tm_addExportScope(void *handle, char *filter, celix_properties_t *props);

/* \brief  remove restricted scope for specified exported service
 *
 * \param  handle pointer to scope
 * \param  filter, filter string
 *
 * \return CELIX_SUCCESS if removed
 *         CELIX_ILLEGAL_ARGUMENT if service not found in scope
 *
 */
celix_status_t tm_removeExportScope(void *handle, char *filter);

/* \brief  add restricted scope for specified imported service
 *
 * \param  handle pointer to scope
 * \param  filter, filter string
 * \param  props additional properties defining restrictions for the imported service
 *
 * \return CELIX_SUCCESS if added to scope
 *         CELIX_ILLEGAL_ARGUMENT if service scope is already restricted before
 *
 */
celix_status_t tm_addImportScope(void *handle, char *filter);


/* \brief  remove restricted scope for specified imported service
 *
 * \param  handle pointer to scope
 * \param  filter, filter string
 *
 * \return CELIX_SUCCESS if removed
 *         CELIX_ILLEGAL_ARGUMENT if service not found in scope
 *
 */
celix_status_t tm_removeImportScope(void *handle, char *filter);


#endif // TOPOLOGY_SCOPE_H_
