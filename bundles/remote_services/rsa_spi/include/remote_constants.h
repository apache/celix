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

#define CELIX_RSA_SERVICE_EXPORTED_INTERFACES "service.exported.interfaces"
#define CELIX_RSA_ENDPOINT_FRAMEWORK_UUID "endpoint.framework.uuid"
#define CELIX_RSA_ENDPOINT_SERVICE_ID "endpoint.service.id"
#define CELIX_RSA_ENDPOINT_ID "endpoint.id"
#define CELIX_RSA_SERVICE_IMPORTED "service.imported"
#define CELIX_RSA_SERVICE_IMPORTED_CONFIGS "service.imported.configs"
#define CELIX_RSA_SERVICE_EXPORTED_CONFIGS "service.exported.configs"
#define CELIX_RSA_SERVICE_LOCATION "service.location"

/**
 * It identify which types of remote service configurations are supported by a distribution provider.
 * @ref https://docs.osgi.org/specification/osgi.cmpn/7.0.0/service.remoteservices.html#i1708968
 */
#define CELIX_RSA_REMOTE_CONFIGS_SUPPORTED "remote.configs.supported"

#endif /* REMOTE_CONSTANTS_H_ */
