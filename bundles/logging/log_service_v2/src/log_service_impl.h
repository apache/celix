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
 * log_service_impl.h
 *
 *  \date       Jun 22, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef LOG_SERVICE_IMPL_H_
#define LOG_SERVICE_IMPL_H_

#include "log_service.h"
#include "log.h"

celix_status_t logService_create(log_t *log, celix_bundle_t *bundle, log_service_data_t **logger);
celix_status_t logService_destroy(log_service_data_t **logger);
celix_status_t logService_log(log_service_data_t *logger, log_level_t level, char * message);
celix_status_t logService_logSr(log_service_data_t *logger, service_reference_pt reference, log_level_t level, char * message);


#endif /* LOG_SERVICE_IMPL_H_ */
