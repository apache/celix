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
 * log_writer_activator.c
 *
 *  \date       Oct 1, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "log_writer.h"

#include "bundle_activator.h"

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	log_writer_pt writer = NULL;

	logWriter_create(context, &writer);

	*userData = writer;

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	log_writer_pt writer = (log_writer_pt) userData;

	return logWriter_start(writer);
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	log_writer_pt writer = (log_writer_pt) userData;

	return logWriter_stop(writer);
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	log_writer_pt writer = (log_writer_pt) userData;

	return logWriter_destroy(&writer);
}
