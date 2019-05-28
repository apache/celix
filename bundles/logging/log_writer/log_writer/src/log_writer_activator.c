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


#include "log_reader_service.h"
#include "log_listener.h"
#include "celix_api.h"
#include "celix_log_writer.h"

typedef struct celix_logwriter_activator {
    celix_log_writer_t *writer;
	long svcTracker;
	log_listener_t *listener;
} celix_logwriter_activator_t;

static void addSvc(void *handle, void *svc) {
    celix_logwriter_activator_t *act = handle;
	log_reader_service_t *reader = svc;
	reader->addLogListener(reader->reader, act->listener);
}

static void remSvc(void *handle, void *svc) {
    celix_logwriter_activator_t *act = handle;
	log_reader_service_t *reader = svc;
    reader->removeLogListener(reader->reader, act->listener);
}

static celix_status_t logWriterActivator_start(celix_logwriter_activator_t *act, celix_bundle_context_t *ctx) {
    act->writer = celix_logWriter_create(ctx);
	act->listener->handle = act->writer;
	act->listener->logged = (void*)celix_logWriter_logged;
	act->svcTracker = -1L;
	if (act->writer != NULL) {
		celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
		opts.filter.serviceName = OSGI_LOGSERVICE_READER_SERVICE_NAME;
		opts.callbackHandle = act;
		opts.add = addSvc;
		opts.remove = remSvc;
		act->svcTracker = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
	}
	return CELIX_SUCCESS;
}

static celix_status_t logWriterActivator_stop(celix_logwriter_activator_t *act, celix_bundle_context_t *ctx) {
	celix_bundleContext_stopTracker(ctx, act->svcTracker);
	celix_logWriter_destroy(act->writer);
	return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(celix_logwriter_activator_t, logWriterActivator_start, logWriterActivator_stop);
