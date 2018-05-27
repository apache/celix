/**
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
/*
 * etcd_watcher.h
 *
 * \date       17 Sep 2014
 * \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 * \copyright  Apache License, Version 2.0
 */

#ifndef ETCD_WATCHER_H_
#define ETCD_WATCHER_H_

#include "celix_errno.h"
#include "discovery.h"
#include "endpoint_discovery_poller.h"

typedef struct etcd_watcher etcd_watcher_t;
typedef struct etcd_watcher *etcd_watcher_pt;

celix_status_t etcdWatcher_create(discovery_pt discovery,  bundle_context_pt context, etcd_watcher_pt *watcher);
celix_status_t etcdWatcher_destroy(etcd_watcher_pt watcher);


#endif /* ETCD_WATCHER_H_ */
