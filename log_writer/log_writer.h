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
 * low_writer.h
 *
 *  Created on: Jul 4, 2011
 *      Author: alexander
 */

#ifndef LOG_WRITER_H_
#define LOG_WRITER_H_

#include "service_component.h"
#include "service_dependency.h"
#include "log_reader_service.h"

struct log_writer {
    log_reader_service_t logReader;
    SERVICE service;
    SERVICE_DEPENDENCY dep;
    log_listener_t logListener;
};

typedef struct log_writer *log_writer_t;

celix_status_t logWriter_create(apr_pool_t *pool, log_writer_t *writer);


#endif /* LOG_WRITER_H_ */
