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
 * shm_watcher.h
 *
 * \date       30 Sep 2014
 * \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 * \copyright  Apache License, Version 2.0
 */

#ifndef DISCOVERY_SHM_WATCHER_H_
#define DISCOVERY_SHM_WATCHER_H_

#include "celix_errno.h"
#include "discovery.h"
#include "endpoint_discovery_poller.h"

typedef struct shm_watcher shm_watcher_t;

celix_status_t discoveryShmWatcher_create(discovery_t *discovery);
celix_status_t discoveryShmWatcher_destroy(discovery_t *discovery);


#endif /* DISCOVERY_SHM_WATCHER_H_ */
