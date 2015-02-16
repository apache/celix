/**
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

/*
 * activator.c
 *
 *  \date       16 Feb 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>

#include <bundle_activator.h>
#include <service_tracker.h>
#include <constants.h>

#include <array_list.h>
#include <pthread.h>

#include "reader_service.h"
#include "reader_service_impl.h"
#include "writer_service.h"
#include "writer_service_impl.h"

#include "database.h"

struct activator {
	database_handler_pt databaseHandler;

	service_registration_pt readerRegistration;
	service_registration_pt writerRegistration;

	reader_service_pt readerService;
	writer_service_pt writerService;
};



celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;

	*userData = calloc (1, sizeof(struct activator));

	if (*userData) {
		((struct activator *) *userData)->readerService = NULL;
		((struct activator *) *userData)->writerService = NULL;
		((struct activator *) *userData)->databaseHandler = NULL;

	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}


celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	reader_service_pt readerService = calloc(1, sizeof(struct reader_service));
	writer_service_pt writerService = calloc(1, sizeof(struct writer_service));
	database_handler_pt databaseHandler = calloc(1, sizeof(struct database_handler));

	if (readerService && writerService && databaseHandler)
	{
	    status = celixThreadMutex_create(&databaseHandler->lock, NULL);

	    if (status == CELIX_SUCCESS)
	    {
            arrayList_create(&databaseHandler->data);
            databaseHandler->dataIndex = 0;

            readerService->handler = databaseHandler;
            readerService->readerService_getFirstData = readerService_getFirstData;
            readerService->readerService_getNextData = readerService_getNextData;

            writerService->handler = databaseHandler;
            writerService->writerService_storeData = writerService_storeData;
            writerService->writerService_removeData = writerService_removeData;

            activator->readerService = readerService;
            activator->writerService = writerService;

            status = bundleContext_registerService(context, READER_SERVICE_NAME, activator->readerService, NULL,  &activator->readerRegistration);

            if (status == CELIX_SUCCESS)
                status = bundleContext_registerService(context, WRITER_SERVICE_NAME, activator->writerService, NULL,  &activator->writerRegistration);
	    }
	}
	else
	{
		status = CELIX_ENOMEM;
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	serviceRegistration_unregister(activator->readerRegistration);
	serviceRegistration_unregister(activator->writerRegistration);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	free(activator->readerService);
	free(activator->writerService);
	free(activator);

	return status;
}

