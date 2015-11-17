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
 * benchmark_activator.c
 *
 *  \date       Feb 12, 2014
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */



#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/queue.h>

#include <pthread.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_registration.h"
#include "service_tracker.h"

#include "benchmark_service.h"
#include "frequency_service.h"

static celix_status_t benchmarkRunner_addingService(void * handle, service_reference_pt reference, void **service);
static celix_status_t benchmarkRunner_addedService(void * handle, service_reference_pt reference, void * service);
static celix_status_t benchmarkRunner_modifiedService(void * handle, service_reference_pt reference, void * service);
static celix_status_t benchmarkRunner_removedService(void * handle, service_reference_pt reference, void * service);

static void benchmarkRunner_runBenchmark(struct activator *activator);
static void benchmarkRunner_printHeader(char *name, unsigned int nrOfSamples);
static void benchmarkRunner_printResult(benchmark_result_t result, double updateFreq, unsigned long elapsedTime);
static void benchmarkRunner_printFooter(char *name);

struct benchmark_entry {
	benchmark_service_pt benchmark;
	LIST_ENTRY(benchmark_entry) entries;
};

struct activator {
	bundle_context_pt context;
	service_tracker_customizer_pt customizer;
	service_tracker_pt tracker;
	pthread_t thread;

	pthread_mutex_t mutex;
	array_list_pt benchmarks;
	LIST_HEAD(benchmark_entries, entries) benchmarkEntries;
	frequency_service_pt freqService;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	struct activator * activator = malloc(sizeof(*activator));
	activator->context=context;
	activator->customizer = NULL;
	activator->tracker= NULL;
	activator->benchmarks = NULL;
	activator->freqService = NULL;

	LIST_INIT(&activator->benchmarkEntries);

	*userData = activator;

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = userData;

	pthread_mutex_init(&activator->mutex, NULL);

	arrayList_create(&activator->benchmarks);

	serviceTrackerCustomizer_create(activator, benchmarkRunner_addingService, benchmarkRunner_addedService, benchmarkRunner_modifiedService, benchmarkRunner_removedService, &activator->customizer);

	char filter[128];
	sprintf(filter, "(|(%s=%s)(%s=%s))", "objectClass", BENCHMARK_SERVICE_NAME, "objectClass", FREQUENCY_SERVICE_NAME);
	serviceTracker_createWithFilter(context, filter, activator->customizer, &activator->tracker);
	serviceTracker_open(activator->tracker);

	pthread_create(&activator->thread, NULL, (void *)benchmarkRunner_runBenchmark, activator);

	return status;
}


celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	struct activator * activator = userData;

	pthread_join(activator->thread, NULL);

	serviceTracker_close(activator->tracker);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	struct activator * activator = userData;

	return CELIX_SUCCESS;
}

static celix_status_t benchmarkRunner_addingService(void * handle, service_reference_pt reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = handle;
	status = bundleContext_getService(activator->context, reference, service);
	return status;

}
static celix_status_t benchmarkRunner_addedService(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = handle;

	service_registration_pt registration = NULL;
	properties_pt properties = NULL;
	char *serviceName = NULL;
	serviceReference_getServiceRegistration(reference, &registration);
	serviceRegistration_getProperties(registration, &properties);
	serviceName = properties_get(properties, "objectClass");
	if (strcmp(serviceName, BENCHMARK_SERVICE_NAME) == 0) {
		pthread_mutex_lock(&activator->mutex);
		arrayList_add(activator->benchmarks, service);
		pthread_mutex_unlock(&activator->mutex);
	} else if (strcmp(serviceName, FREQUENCY_SERVICE_NAME) == 0 ) {
		pthread_mutex_lock(&activator->mutex);
		activator->freqService = service;
		pthread_mutex_unlock(&activator->mutex);
	}

	return status;
}
static celix_status_t benchmarkRunner_modifiedService(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = handle;
	return status;
}

static celix_status_t benchmarkRunner_removedService(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = handle;

	service_registration_pt registration = NULL;
		properties_pt properties = NULL;
		char *serviceName = NULL;
		serviceReference_getServiceRegistration(reference, &registration);
		serviceRegistration_getProperties(registration, &properties);
		serviceName = properties_get(properties, "objectClass");
		if (strcmp(serviceName, BENCHMARK_SERVICE_NAME) == 0) {
			pthread_mutex_lock(&activator->mutex);
			arrayList_removeElement(activator->benchmarks, service);
			pthread_mutex_unlock(&activator->mutex);
		} else if (strcmp(serviceName, FREQUENCY_SERVICE_NAME) == 0 ) {
			pthread_mutex_lock(&activator->mutex);
			if (activator->freqService == service) {
				activator->freqService = NULL;
			}
			pthread_mutex_unlock(&activator->mutex);
		}

	return status;
}

static void benchmarkRunner_runBenchmark(struct activator *activator) {
	int i, k;
	int nrOfBenchmarks;
	double updateFrequency, measuredFrequency;
	unsigned int measuredUpdateCounter, nrOfUpdateThreads;
	int nrOfSamples;
	benchmark_service_pt benchmarkServ;
	char *name;
	benchmark_result_t result;
	struct timeval begin,end;
	unsigned long elapsedTime;
	double sampleFactor;

	int nrOfThreadRuns = 12;
	int threads[] = {1,2,3,4,5,6,7,8,16,32,64,128};

	nrOfSamples = 100 * 1000;
	updateFrequency = 1000;
	nrOfUpdateThreads = 100;

	usleep(2000 * 1000); //wait 2 seconds to get needed services

	pthread_mutex_lock(&activator->mutex);
	if (activator->freqService != NULL) {
		activator->freqService->setFrequency(activator->freqService->handle, updateFrequency);
		activator->freqService->setNrOfThreads(activator->freqService->handle, nrOfUpdateThreads);
	}
	nrOfBenchmarks = arrayList_size(activator->benchmarks);
	for (i = 0 ; i < nrOfBenchmarks ; i += 1) {
		benchmarkServ = arrayList_get(activator->benchmarks, i);
		name = benchmarkServ->name(benchmarkServ->handler);
		sampleFactor = benchmarkServ->getSampleFactor(benchmarkServ->handler);
		activator->freqService->setBenchmarkName(activator->freqService->handle, name);
		usleep(1000);
		benchmarkRunner_printHeader(name, nrOfSamples * sampleFactor);
		for (k = 0 ; k < nrOfThreadRuns ; k +=1) {
			if (activator->freqService != NULL) {
					activator->freqService->resetCounter(activator->freqService->handle);

			}
			gettimeofday(&begin, NULL);
			result = benchmarkServ->run(benchmarkServ->handler, threads[k], nrOfSamples * sampleFactor);
			gettimeofday(&end, NULL);
			elapsedTime = ((end.tv_sec - begin.tv_sec) * 1000000) + (end.tv_usec - begin.tv_usec);
			if (activator->freqService != NULL) {
				measuredUpdateCounter = activator->freqService->getCounter(activator->freqService->handle);
				measuredFrequency = ((double)(measuredUpdateCounter) / elapsedTime * 1000000);
			}
			benchmarkRunner_printResult(result, measuredFrequency, elapsedTime);
		}
		benchmarkRunner_printFooter(name);
	}
	pthread_mutex_unlock(&activator->mutex);
}

static void benchmarkRunner_printHeader(char *name, unsigned int nrOfSamples) {
		int i;
		printf("---% 35s---------------------------------------------------------------------------------------\n", name);
		printf("-------samples: %10i---------------------------------------------------------------------------------------------------\n", nrOfSamples);
}

static void benchmarkRunner_printResult(benchmark_result_t result, double updateFreq, unsigned long elapsedTime) {
	printf("| threads %5i | ", result.nrOfThreads);
	printf("average call time: % 10.2f nanoseconds | ", result.averageCallTimeInNanoseconds);
	printf("frequency calls is % 10.5f MHz | ", result.callFrequencyInMhz);
	printf("update freq ~ % 8.2f Hz | ", updateFreq);
	printf("elapsed time is % 8.5f seconds | ", ((double)elapsedTime) / 1000000);
	printf("\n");
}

static void benchmarkRunner_printFooter(char *name) {
	printf("-----------------------------------------------------------------------------------------------------------------------------\n\n\n");
}
