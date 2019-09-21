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
 * updated_thread_pool.h
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef UPDATED_THREAD_POOL_H_
#define UPDATED_THREAD_POOL_H_


#define INIT_THREADS 0
#define MAX_THREADS 10

typedef struct updated_thread_pool *updated_thread_pool_pt;

/* celix.framework.public */
#include "bundle_context.h"
#include "celix_errno.h"
#include "properties.h"
/* celix.config_admin.public*/
#include "managed_service.h"

celix_status_t updatedThreadPool_create( bundle_context_pt context, int maxTreads, updated_thread_pool_pt *updatedThreadPool);
celix_status_t updatedThreadPool_destroy(updated_thread_pool_pt pool);
celix_status_t updatedThreadPool_push(updated_thread_pool_pt updatedThreadPool, managed_service_service_pt service, properties_pt properties);


#endif /* UPDATED_THREAD_POOL_H_ */
