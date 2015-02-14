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
 * log_writer.c
 *
 *  \date       Mar 7, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "celix_errno.h"
#include "celixbool.h"

#include "log_writer.h"
#include "log_listener.h"
#include "module.h"
#include "bundle.h"

celix_status_t logWriter_create(bundle_context_pt context, log_writer_pt *writer) {
	celix_status_t status = CELIX_SUCCESS;

	*writer = calloc(1, sizeof(**writer));
	(*writer)->logListener = calloc(1, sizeof(*(*writer)->logListener));
	(*writer)->logListener->handle = *writer;
	(*writer)->logListener->logged = logListener_logged;
	(*writer)->logReader = NULL;
	(*writer)->context = context;
	(*writer)->tracker = NULL;

	return status;
}


celix_status_t logWriter_destroy(log_writer_pt *writer) {
	celix_status_t status = CELIX_SUCCESS;

	free((*writer)->logListener);
	free(*writer);

	writer = NULL;

	return status;
}
celix_status_t logWriter_start(log_writer_pt writer) {
	celix_status_t status = CELIX_SUCCESS;

	service_tracker_customizer_pt cust = NULL;
	service_tracker_pt tracker = NULL;

	status = serviceTrackerCustomizer_create(writer, logWriter_addingServ, logWriter_addedServ, logWriter_modifiedServ, logWriter_removedServ, &cust);
	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(writer->context, (char *) OSGI_LOGSERVICE_READER_SERVICE_NAME, cust, &tracker);
		if (status == CELIX_SUCCESS) {
			writer->tracker = tracker;
			status = serviceTracker_open(tracker);
		}
	}

	return status;
}

celix_status_t logWriter_stop(log_writer_pt writer) {
	celix_status_t status = CELIX_SUCCESS;

	if (serviceTracker_close(writer->tracker) != CELIX_SUCCESS) {
		status = CELIX_BUNDLE_EXCEPTION;
	}
	if (serviceTracker_destroy(writer->tracker) != CELIX_SUCCESS) {
		status = CELIX_BUNDLE_EXCEPTION;
	}

	return status;
}

celix_status_t logWriter_addingServ(void * handle, service_reference_pt ref, void **service) {
	log_writer_pt writer = (log_writer_pt) handle;
	bundleContext_getService(writer->context, ref, service);
	return CELIX_SUCCESS;
}

celix_status_t logWriter_addedServ(void * handle, service_reference_pt ref, void * service) {
	log_writer_pt writer = (log_writer_pt) handle;

	// Add this writer to each log reader service found
	if (service != NULL) {
		((log_reader_service_pt) service)->addLogListener(((log_reader_service_pt) service)->reader, writer->logListener);
	}

	return CELIX_SUCCESS;
}

celix_status_t logWriter_modifiedServ(void * handle, service_reference_pt ref, void * service) {
	return CELIX_SUCCESS;
}

celix_status_t logWriter_removedServ(void * handle, service_reference_pt ref, void * service) {
	log_writer_pt writer = (log_writer_pt) handle;

	if (service != NULL) {
		((log_reader_service_pt) service)->removeLogListener(((log_reader_service_pt) service)->reader, writer->logListener);
	}

	return CELIX_SUCCESS;
}
