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
 * dm_server_impl.h
 *
 *  \date       16 Oct 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "dm_server.h"
#include "dm_component.h"
#include "dm_component_impl.h"
#include "dm_dependency_manager_impl.h"

celix_status_t dmServiceCreate(dm_server_pt * dmServ, bundle_context_pt context, struct dm_dependency_manager *manager);
celix_status_t dmServiceAddComponent(dm_server_pt dmServ, dm_component_pt component);
celix_status_t dmServiceDestroy(dm_server_pt dmServ);
celix_status_t dmService_getInfo(dm_server_pt dmServ, dm_info_pt info);

