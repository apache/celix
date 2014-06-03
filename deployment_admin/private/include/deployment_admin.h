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
 * deployment_admin.h
 *
 *  \date       Nov 7, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef DEPLOYMENT_ADMIN_H_
#define DEPLOYMENT_ADMIN_H_

#include <apr_thread_proc.h>

#include "bundle_context.h"

typedef struct deployment_admin *deployment_admin_pt;

struct deployment_admin {
	apr_pool_t *pool;
	apr_thread_t *poller;
	bundle_context_pt context;

	bool running;
	char *current;
	hash_map_pt packages;
	char *targetIdentification;
	char *pollUrl;
	char *auditlogUrl;
	apr_time_t auditlogId;
	unsigned int aditlogSeqNr;
};

typedef enum {
	DEPLOYMENT_ADMIN_AUDIT_EVENT__FRAMEWORK_STARTED = 1005
} DEPLOYMENT_ADMIN_AUDIT_EVENT;

celix_status_t deploymentAdmin_create(apr_pool_t *pool, bundle_context_pt context, deployment_admin_pt *admin);

#endif /* DEPLOYMENT_ADMIN_H_ */
