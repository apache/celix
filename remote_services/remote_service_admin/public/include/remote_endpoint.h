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
 * remote_endpoint.h
 *
 *  \date       Oct 7, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REMOTE_ENDPOINT_H_
#define REMOTE_ENDPOINT_H_

#define OSGI_RSA_REMOTE_ENDPOINT "remote_endpoint"

typedef struct remote_endpoint *remote_endpoint_pt;

struct remote_endpoint_service {
	remote_endpoint_pt endpoint;
	celix_status_t (*setService)(remote_endpoint_pt endpoint, void *service);
	celix_status_t (*handleRequest)(remote_endpoint_pt endpoint, char *data, char **reply);
};

typedef struct remote_endpoint_service *remote_endpoint_service_pt;


#endif /* REMOTE_ENDPOINT_H_ */
