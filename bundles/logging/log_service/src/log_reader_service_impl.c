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
 * log_reader_service_impl.c
 *
 *  \date       Jun 26, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <stddef.h>

#include "log_reader_service_impl.h"
#include "celixbool.h"

struct log_reader_data {
    log_t *log;
};

celix_status_t logReaderService_create(log_t *log, log_reader_data_t **reader) {
    celix_status_t status = CELIX_SUCCESS;

    *reader = (log_reader_data_t *) calloc(1, sizeof(**reader));

    if (*reader == NULL) {
        status = CELIX_ENOMEM;
    } else {
        (*reader)->log = log;
    }

    return status;
}

celix_status_t logReaderService_destroy(log_reader_data_t **reader) {
    celix_status_t status = CELIX_SUCCESS;

    free(*reader);
    reader = NULL;

    return status;
}



celix_status_t logReaderService_getLog(log_reader_data_t *reader, linked_list_pt *list) {
    celix_status_t status = CELIX_SUCCESS;

    status = log_getEntries(reader->log, list);

    return status;
}

celix_status_t logReaderService_addLogListener(log_reader_data_t *reader, log_listener_t *listener) {
    return log_addLogListener(reader->log, listener);
}

celix_status_t logReaderService_removeLogListener(log_reader_data_t *reader, log_listener_t *listener) {
    return log_removeLogListener(reader->log, listener);
}

celix_status_t logReaderService_removeAllLogListener(log_reader_data_t *reader) {
    return log_removeAllLogListener(reader->log);
}


