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
 * remote_service_admin_http_impl.h
 *
 *  \date       Sep 30, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REMOTE_SERVICE_ADMIN_HTTP_IMPL_H_
#define REMOTE_SERVICE_ADMIN_HTTP_IMPL_H_

#include "remote_service_admin_impl.h"
#include "log_helper.h"
#include "civetweb.h"

struct remote_service_admin {
	apr_pool_t *pool;
	bundle_context_pt context;
	log_helper_pt loghelper;

	hash_map_pt exportedServices;
	hash_map_pt importedServices;

	char *port;
	char *ip;

	struct mg_context *ctx;
};

celix_status_t remoteServiceAdmin_stop(remote_service_admin_pt admin);

#endif /* REMOTE_SERVICE_ADMIN_HTTP_IMPL_H_ */
