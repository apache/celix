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

#ifndef CELIX_LOG_WRITER_H_
#define CELIX_LOG_WRITER_H_


#include "celix_api.h"
#include "log_listener.h"

typedef struct celix_log_writer celix_log_writer_t; //opaque pointer

celix_log_writer_t* celix_logWriter_create(celix_bundle_context_t *ctx);
void celix_logWriter_destroy(celix_log_writer_t *writer);
celix_status_t celix_logWriter_logged(celix_log_writer_t *writer, log_entry_t *entry);

#endif /* CELIX_LOG_WRITER_H_ */
