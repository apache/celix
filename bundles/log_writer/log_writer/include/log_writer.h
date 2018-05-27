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
 * log_writer.h
 *
 *  \date       Jul 4, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef LOG_WRITER_H_
#define LOG_WRITER_H_

#include "log_reader_service.h"
#include "service_tracker.h"

struct log_writer {
    log_reader_service_pt logReader;
    log_listener_pt logListener;

    bundle_context_pt context;
    service_tracker_pt tracker;
};

typedef struct log_writer *log_writer_pt;

celix_status_t logWriter_create(bundle_context_pt context, log_writer_pt *writer);
celix_status_t logWriter_destroy(log_writer_pt *writer);
celix_status_t logWriter_start(log_writer_pt writer);
celix_status_t logWriter_stop(log_writer_pt writer);

celix_status_t logWriter_addingServ(void * handle, service_reference_pt ref, void **service);
celix_status_t logWriter_addedServ(void * handle, service_reference_pt ref, void * service);
celix_status_t logWriter_modifiedServ(void * handle, service_reference_pt ref, void * service);
celix_status_t logWriter_removedServ(void * handle, service_reference_pt ref, void * service);

#endif /* LOG_WRITER_H_ */
