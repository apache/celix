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

#include "celix_api.h"

typedef struct deprecated_log_writer_stdout_activator {
    //nop
} deprecated_log_writer_stdout_activator_t;

celix_status_t logWriterStdOut_start(deprecated_log_writer_stdout_activator_t* act  __attribute__((unused)), celix_bundle_context_t* ctx  __attribute__((unused))) {
    fprintf(stderr, "Celix::log_writer_stdout bundle is deprecated. Please use Celix::log_admin instead of Celix::log_service and Celix::log_writer_stdout\n");
    return CELIX_SUCCESS;
}

celix_status_t logWriterStdOut_stop(deprecated_log_writer_stdout_activator_t* act  __attribute__((unused)), celix_bundle_context_t* ctx  __attribute__((unused))) {
    //nop;
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(deprecated_log_writer_stdout_activator_t, logWriterStdOut_start, logWriterStdOut_stop)