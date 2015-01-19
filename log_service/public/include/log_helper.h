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


#ifndef LOGHELPER_H_
#define LOGHELPER_H_

#include "bundle_context.h"
#include "log_service.h"

typedef struct log_helper* log_helper_pt;

celix_status_t logHelper_create(bundle_context_pt context, log_helper_pt* log_helper);
celix_status_t logHelper_start(log_helper_pt loghelper);
celix_status_t logHelper_stop(log_helper_pt loghelper);
celix_status_t logHelper_destroy(log_helper_pt* loghelper);
celix_status_t logHelper_log(log_helper_pt loghelper, log_level_t level, char* message, ... );

#endif /* LOGHELPER_H_ */
