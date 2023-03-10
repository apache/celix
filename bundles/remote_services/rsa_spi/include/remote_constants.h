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
 * remote_constants.h
 *
 *  \date       Sep 30, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REMOTE_CONSTANTS_H_
#define REMOTE_CONSTANTS_H_

static const char * const OSGI_RSA_SERVICE_EXPORTED_INTERFACES = "service.exported.interfaces";
static const char * const OSGI_RSA_ENDPOINT_FRAMEWORK_UUID = "endpoint.framework.uuid";
static const char * const OSGI_RSA_ENDPOINT_SERVICE_ID = "endpoint.service.id";
static const char * const OSGI_RSA_ENDPOINT_ID = "endpoint.id";
static const char * const OSGI_RSA_SERVICE_IMPORTED = "service.imported";
static const char * const OSGI_RSA_SERVICE_IMPORTED_CONFIGS = "service.imported.configs";
static const char * const OSGI_RSA_SERVICE_EXPORTED_CONFIGS = "service.exported.configs";
static const char * const OSGI_RSA_SERVICE_LOCATION = "service.location";

/**
 * In a multi-network interface environment，RSA can use RSA_DZC_IF_INDEX to specify
 * on which interface its exported services can be discovered.
 * If RSA unset 'RSA_DZC_IF_INDEX', its exported service will be discovered
 * only by other local clients on the same machine
 */
static const char * const RSA_DISCOVERY_ZEROCONF_SERVICE_ANNOUNCED_IF_INDEX = "RSA_DZC_IF_INDEX";

#endif /* REMOTE_CONSTANTS_H_ */
