/*
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

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "benchmark.h"

static const char * const BENCHMARK_NAME = "REFERENCE";
static const double SAMPLE_FACTOR = 100;

typedef struct thread_info {
	benchmark_pt benchmark;
	int nrOfSamples;
	unsigned int result;
	struct timeval begin;
	struct timeval end;
	int skips;
} thread_info_t;

static void benchmark_thread(thread_info_t *info);
static int benchmark_calc(int arg1, int arg2);

celix_status_t benchmark_create(benchmark_pt *benchmark) {
	//do nothing
	return CELIX_SUCCESS;
}

celix_status_t benchmark_destroy(benchmark_pt benchmark) {
	//do nothing
	return CELIX_SUCCESS;
}

benchmark_result_t benchmark_run(benchmark_pt benchmark, int nrOfThreads, int nrOfSamples) {
	int i;
	pthread_t threads[nrOfThreads];
	thread_info_t infos[nrOfThreads];
	benchmark_result_t result;
	unsigned long elapsedTime = 0;

	result.skips =0;

	for (i = 0 ; i < nrOfThreads ; i += 1) {
		infos[i].benchmark = benchmark;
		infos[i].nrOfSamples = nrOfSamples;
		infos[i].skips = 0;
		infos[i].result = rand();
		pthread_create(&threads[i], NULL, (void *)benchmark_thread,  &infos[i]);
	}

	for (i = 0; i < nrOfThreads ; i += 1) {
		pthread_join(threads[i], NULL);
		elapsedTime += ((infos[i].end.tv_sec - infos[i].begin.tv_sec) * 1000000) + (infos[i].end.tv_usec - infos[i].begin.tv_usec);
		result.skips += infos[i].skips;
	}

	result.averageCallTimeInNanoseconds = elapsedTime;
	result.averageCallTimeInNanoseconds *= 1000;
	result.averageCallTimeInNanoseconds /= nrOfSamples;
	result.averageCallTimeInNanoseconds /= nrOfThreads;
	result.callFrequencyInMhz = ((double)(nrOfSamples * nrOfThreads) / elapsedTime);
	result.nrOfThreads = nrOfThreads;
	result.nrOfsamples = nrOfSamples;

	return result;
}

static void benchmark_thread(thread_info_t *info) {
	int i;

	int result = info->result;
	struct timeval *begin = &info->begin;
	struct timeval *end = &info->end;
	int nrOFSamples = info->nrOfSamples;


	gettimeofday(begin, NULL);
	for (i = 0; i < nrOFSamples; i += 1) {
		result = benchmark_calc(result, i);
	}
	gettimeofday(end, NULL);

	info->result = result;
}

char * benchmark_getName(benchmark_pt benchmark) {
	return (char *)BENCHMARK_NAME;
}

celix_status_t benchmark_addMathService(benchmark_pt benchmark, math_service_pt mathService) {
	//ignore service is not used
	return CELIX_SUCCESS;
}

celix_status_t benchmark_removeMathService(benchmark_pt benchmark, math_service_pt mathService) {
	//ignore service is not used
	return CELIX_SUCCESS;

}

/*
 * Same implementation as the math_service. This function is used a reference.
 */
static int benchmark_calc(int arg1, int arg2) {
	return  arg1 * arg2 + arg2;
}

double benchmark_getSampleFactor(benchmark_pt benchmark) {
	return SAMPLE_FACTOR;
}
