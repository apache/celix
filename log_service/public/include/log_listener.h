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
 * log_listener.h
 *
 *  \date       Jul 4, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef LOG_LISTENER_H_
#define LOG_LISTENER_H_

#include "log_entry.h"
#include "celix_errno.h"

struct log_listener {
    void *handle;
    celix_status_t (*logged)(struct log_listener *listener, log_entry_pt entry);
};

typedef struct log_listener * log_listener_pt;

celix_status_t logListener_logged(log_listener_pt listener, log_entry_pt entry);

#endif /* LOG_LISTENER_H_ */
