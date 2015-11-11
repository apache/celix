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
 * echo_server_activator.c
 *
 *  \date       Sep 21, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_registration.h"

#include "math_service.h"
#include "frequency_service.h"
#include "math_component.h"

typedef struct activator {
	bundle_context_pt context;

	frequency_service_pt freqService;
	service_registration_pt freqRegistration;

	math_component_pt math;
	math_service_pt mathService;
	char *benchmarkName;
	service_registration_pt registration;

	uint updateFrequency;
	uint nrOfThreads;
	pthread_t *threads;


	volatile uint counter;
	struct timeval beginMeasurement;
	struct timeval endMeasurement;
} activator_t;

static int calc(int arg1, int arg2);
static void run(activator_t *activator);
static void setFrequency(activator_t *activator, uint freq);
static void setNrOfThreads(activator_t *activator, uint nrOfThreads);
static void resetCounter(activator_t *activator);
static void stopThreads(activator_t *activator);
static void startThreads(activator_t *activator, uint nrOfThreads);
static uint getCounter(activator_t *activator);
static void setBenchmarkName(activator_t *activator, char *benchmark);
static math_service_pt registerMath(activator_t *activator, service_registration_pt *reg);

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	activator_t * activator = malloc(sizeof(*activator));
	activator->context = context;
	activator->benchmarkName = NULL;
	activator->freqService  = NULL;
	activator->registration = NULL;
	activator->freqRegistration  = NULL;
	activator->updateFrequency = 0;
	activator->nrOfThreads = 0;
	activator->math = NULL;

	mathComponent_create(&activator->math);

	*userData = activator;

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = userData;

	activator->mathService = malloc(sizeof(*activator->mathService));
	activator->mathService->handle = activator->math;
	activator->mathService->calc = (void *)mathComponent_calc;
	bundleContext_registerService(activator->context, MATH_SERVICE_NAME, activator->mathService, NULL, &activator->registration);

	activator->freqService = malloc(sizeof(*activator->freqService));
	activator->freqService->handle = (void *)activator;
	activator->freqService->setFrequency = (void *)setFrequency;
	activator->freqService->resetCounter = (void *)resetCounter;
	activator->freqService->getCounter = (void *)getCounter;
	activator->freqService->setBenchmarkName = (void *)setBenchmarkName;
	activator->freqService->setNrOfThreads = (void *)setNrOfThreads;
	bundleContext_registerService(activator->context, FREQUENCY_SERVICE_NAME, activator->freqService, NULL, &activator->freqRegistration);

	startThreads(activator, activator->nrOfThreads);

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	struct activator * activator = userData;

	printf("Stopping service registration thread\n");
	stopThreads(activator);

	//TODO deregister service & freqService

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	struct echoActivator * activator = userData;

	//TODO free service & freqService struct

	free(activator);

	return CELIX_SUCCESS;
}

static int calc(int arg1, int arg2) {
	return  arg1 * arg2 + arg2;
}

static void stopThreads(activator_t *activator) {
	//cancel and join threads
	if (activator->threads != NULL) {
		for (int i = 0 ; i < activator->nrOfThreads ; i += 1) {
			pthread_cancel(activator->threads[i]);
			pthread_join(activator->threads[i], NULL);
		}
	}
}

static void startThreads(activator_t *activator, uint nrOfThreads) {
	activator->threads = malloc(sizeof(pthread_t) * nrOfThreads);
	for (int i = 0 ; i < nrOfThreads ; i += 1) {
		pthread_create(&activator->threads[i], NULL, (void *)run, activator);
	}
	activator->nrOfThreads = nrOfThreads;
}

static void run(activator_t *activator) {
	service_registration_pt currentReg = NULL;
	service_registration_pt prevReg = NULL;
	math_service_pt current = NULL;
	math_service_pt prev = NULL;
	while (1) {
		pthread_testcancel(); //NOTE no clean exit still need to clear a register service
 		uint delayInMicroseconds =  activator->updateFrequency == 0 ? 0 : (1000 * 1000) / activator->updateFrequency;
		if (delayInMicroseconds > 0) {
			prevReg = currentReg;
			prev = current;

			currentReg = NULL;
			current = registerMath(activator, &currentReg);

			if (prevReg != NULL) {
				serviceRegistration_unregister(prevReg);
				free(prev);
			}
		}
		usleep(delayInMicroseconds > 0 ? delayInMicroseconds : 1000000);
	}
}

static math_service_pt registerMath(activator_t *activator, service_registration_pt *reg) {
	math_service_pt serv = NULL;
	serv = malloc(sizeof(*activator->mathService));
	serv->handle = activator->math;
	serv->calc = (void *)mathComponent_calc;
	properties_pt props = properties_create();
	if (activator->benchmarkName != NULL) { //TODO FIXME race condition
		properties_set(props, "benchmark", activator->benchmarkName);
	}
	bundleContext_registerService(activator->context, MATH_SERVICE_NAME,
			serv, props, reg);
	activator->counter += 1;
	return serv;
}

static void setBenchmarkName(activator_t *activator, char *benchmark) {
	char *old = activator->benchmarkName;
	activator->benchmarkName = strdup(benchmark);
	free(old);
	if (activator->updateFrequency == 0) {
		service_registration_pt reg = NULL;
		registerMath(activator, &reg); //TODO service will not be cleaned up !
	}
}

static void setFrequency(activator_t *activator, uint freq) {
	printf("Setting frequency to %i\n", freq);
	activator->updateFrequency = freq;
}

static void setNrOfThreads(activator_t *activator, uint nrOfThreads) {
	stopThreads(activator);
	startThreads(activator, nrOfThreads);
}

static void resetCounter(activator_t *activator) {
	activator->counter = 0;
}

static uint getCounter(activator_t *activator) {
	return activator->counter;
}
