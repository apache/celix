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

#include <pthread.h>
#include <string.h>
#include <assert.h>

#include "command.h"

#include "bundle_activator.h"
#include "service_tracker.h"

#include "benchmark_service.h"
#include "frequency_service.h"

static celix_status_t benchmarkRunner_addingService(void * handle, service_reference_pt reference, void **service);
static celix_status_t benchmarkRunner_addedService(void * handle, service_reference_pt reference, void * service);
static celix_status_t benchmarkRunner_modifiedService(void * handle, service_reference_pt reference, void * service);
static celix_status_t benchmarkRunner_removedService(void * handle, service_reference_pt reference, void * service);

static void benchmarkRunner_runBenchmark(struct activator *activator, FILE *out);
static void benchmarkRunner_printHeader(char *name, unsigned int nrOfSamples, FILE *out);
static void benchmarkRunner_printResult(benchmark_result_t result, double updateFreq, unsigned long elapsedTime,
                                       FILE *out);
static void benchmarkRunner_printFooter(char *name, FILE *out);
static celix_status_t benchmarkRunner_execute(void *handle, char * line, FILE *out, FILE *err);


struct activator {
	bundle_context_pt context;
	service_tracker_customizer_pt customizer;
	service_tracker_pt tracker;

	pthread_mutex_t mutex;
	benchmark_service_pt benchmark;
	frequency_service_pt freqService;

    command_service_pt command;
    service_registration_pt reg;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	struct activator * activator = malloc(sizeof(*activator));
	activator->context=context;
	activator->customizer = NULL;
	activator->tracker= NULL;
	activator->benchmark = NULL;
	activator->freqService = NULL;

	*userData = activator;

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = userData;

	pthread_mutex_init(&activator->mutex, NULL);

	serviceTrackerCustomizer_create(activator, benchmarkRunner_addingService, benchmarkRunner_addedService, benchmarkRunner_modifiedService, benchmarkRunner_removedService, &activator->customizer);

	char filter[128];
	sprintf(filter, "(|(%s=%s)(%s=%s))", "objectClass", BENCHMARK_SERVICE_NAME, "objectClass", FREQUENCY_SERVICE_NAME);
	serviceTracker_createWithFilter(context, filter, activator->customizer, &activator->tracker);
	serviceTracker_open(activator->tracker);


    activator->reg = NULL;
    activator->command = calloc(1, sizeof(*activator->command));
    activator->command->handle = activator;
    activator->command->executeCommand = benchmarkRunner_execute;
    properties_pt props = properties_create();
    properties_set(props, OSGI_SHELL_COMMAND_NAME, "benchmark");
    properties_set(props, OSGI_SHELL_COMMAND_USAGE, "benchmark run");
    properties_set(props, OSGI_SHELL_COMMAND_DESCRIPTION, "run the available benchmark");

    bundleContext_registerService(context, (char *)OSGI_SHELL_COMMAND_SERVICE_NAME, activator->command, props, &activator->reg);

	return status;
}


celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context __attribute__((unused))) {
	struct activator * activator = userData;

	serviceTracker_close(activator->tracker);

    serviceRegistration_unregister(activator->reg);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context __attribute__((unused))) {
	struct activator * activator = userData;
    free(activator);
	return CELIX_SUCCESS;
}

static celix_status_t benchmarkRunner_addingService(void * handle, service_reference_pt reference, void **service) {
	celix_status_t status;
	struct activator * activator = handle;
	status = bundleContext_getService(activator->context, reference, service);
	return status;

}
static celix_status_t benchmarkRunner_addedService(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = handle;

	service_registration_pt registration = NULL;
	properties_pt properties = NULL;
	const char* serviceName = NULL;
	serviceReference_getServiceRegistration(reference, &registration);
	serviceRegistration_getProperties(registration, &properties);
	serviceName = properties_get(properties, "objectClass");
	if (strcmp(serviceName, BENCHMARK_SERVICE_NAME) == 0) {
		pthread_mutex_lock(&activator->mutex);
		activator->benchmark = service;
		pthread_mutex_unlock(&activator->mutex);
	} else if (strcmp(serviceName, FREQUENCY_SERVICE_NAME) == 0 ) {
		pthread_mutex_lock(&activator->mutex);
		activator->freqService = service;
		pthread_mutex_unlock(&activator->mutex);
	}

	return status;
}
static celix_status_t benchmarkRunner_modifiedService(void * handle __attribute__((unused)), service_reference_pt reference __attribute__((unused)), void * service __attribute__((unused))) {
	celix_status_t status = CELIX_SUCCESS;
	//ignore
	return status;
}

static celix_status_t benchmarkRunner_removedService(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator * activator = handle;

	service_registration_pt registration = NULL;
		properties_pt properties = NULL;
		const char* serviceName = NULL;
		serviceReference_getServiceRegistration(reference, &registration);
		serviceRegistration_getProperties(registration, &properties);
		serviceName = properties_get(properties, "objectClass");
		if (strcmp(serviceName, BENCHMARK_SERVICE_NAME) == 0) {
			pthread_mutex_lock(&activator->mutex);
			if (activator->benchmark == service) {
				activator->benchmark = NULL;
			}
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

static void benchmarkRunner_runBenchmark(struct activator *activator, FILE *out) {
	int k;
	double updateFrequency;
    double measuredFrequency = 0.0;
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

	benchmarkServ = activator->benchmark;
	name = benchmarkServ->name(benchmarkServ->handler);
	sampleFactor = benchmarkServ->getSampleFactor(benchmarkServ->handler);

	pthread_mutex_lock(&activator->mutex);
	if (activator->freqService != NULL) {
		activator->freqService->setFrequency(activator->freqService->handle, updateFrequency);
		activator->freqService->setNrOfThreads(activator->freqService->handle, nrOfUpdateThreads);
		activator->freqService->setBenchmarkName(activator->freqService->handle, name);
	}

	usleep(1000);
    benchmarkRunner_printHeader(name, nrOfSamples * (int)sampleFactor, out);
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
        benchmarkRunner_printResult(result, measuredFrequency, elapsedTime, out);
	}
    benchmarkRunner_printFooter(name, out);

	pthread_mutex_unlock(&activator->mutex);
}

static void benchmarkRunner_printHeader(char *name, unsigned int nrOfSamples, FILE *out) {
		fprintf(out, "---%35s---------------------------------------------------------------------------------------\n", name);
		fprintf(out, "-------samples: %10i---------------------------------------------------------------------------------------------------\n", nrOfSamples);
}

static void benchmarkRunner_printResult(benchmark_result_t result, double updateFreq, unsigned long elapsedTime,
                                       FILE *out) {
	fprintf(out, "| threads %5i | ", result.nrOfThreads);
	fprintf(out, "average call time: %10.2f nanoseconds | ", result.averageCallTimeInNanoseconds);
	fprintf(out, "frequency calls is %10.5f MHz | ", result.callFrequencyInMhz);
	fprintf(out, "update freq ~ %8.2f Hz | ", updateFreq);
	fprintf(out, "elapsed time is %8.5f seconds | ", ((double)elapsedTime) / 1000000);
    if (result.skips > 0 ) {
        fprintf(out, "Warning skipped %i calls", result.skips);
    }
	fprintf(out, "\n");
}

static void benchmarkRunner_printFooter(char *name __attribute__((unused)), FILE *out) {
	fprintf(out, "-----------------------------------------------------------------------------------------------------------------------------\n\n\n");
}

static celix_status_t benchmarkRunner_execute(void *handle, char * line, FILE *out, FILE *err) {
    struct activator * activator = handle;
    char *savePtr;
    char *token = NULL;
    token = strtok_r(line, " ", &savePtr); //command name
    assert(strcmp(token, "benchmark") == 0);
    token = strtok_r(NULL, " ", &savePtr); //sub command
    if (strcmp("run", token) == 0) {
        token = strtok_r(NULL, " ", &savePtr); //possible nr of time to run
        int times = 1;
        int i;
        if (token != NULL) {
            times = atoi(token);
        }
        for (i = 0; i < times; i += 1) {
            fprintf(out, "running benchmark %i of %i\n", i+1, times);
            benchmarkRunner_runBenchmark(activator, out);
        }
    } else {
        fprintf(err, "Unknown subcommand '%s'\n", token);
    }

    return CELIX_SUCCESS;
}
