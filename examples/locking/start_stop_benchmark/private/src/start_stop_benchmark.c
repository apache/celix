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
#include <stdio.h>

#include "benchmark.h"

static const char * const BENCHMARK_NAME = "INTR_CONT";
static const double SAMPLE_FACTOR = 100;
static const __useconds_t WAIT_TIME = 1; //100 * 1000;

typedef enum benchmark_state {
	BENCHMARK_STATE_INTERRUPTED,
	BENCHMARK_STATE_RUNNING
} benchmark_state_t;

struct benchmark {
	int nrOfThreads;
	pthread_mutex_t mutex; //write protect for state
	math_service_pt math;
	benchmark_state_t state;
	int threadsRunning;
};

typedef struct thread_info {
	benchmark_pt benchmark;
	int nrOfSamples;
	unsigned int result;
	struct timeval begin;
	struct timeval end;
	int skips;
} thread_info_t;

static void benchmark_thread(thread_info_t *info);
static void benchmark_runSamples(thread_info_t *info, int *i, volatile benchmark_state_t *state);
static void benchmark_interrupt(benchmark_pt benchmark);
static void benchmark_continue(benchmark_pt benchmark);

celix_status_t benchmark_create(benchmark_pt *benchmark) {
	(*benchmark) = malloc(sizeof(struct benchmark));
	(*benchmark)->math = NULL;
	(*benchmark)->state = BENCHMARK_STATE_INTERRUPTED;
	(*benchmark)->nrOfThreads = 0;
	(*benchmark)->threadsRunning = 0;

	pthread_mutex_init(&(*benchmark)->mutex, NULL);

	return CELIX_SUCCESS;
}

celix_status_t benchmark_destroy(benchmark_pt benchmark) {
	free(benchmark);

	return CELIX_SUCCESS;
}

benchmark_result_t benchmark_run(benchmark_pt benchmark, int nrOfThreads, int nrOfSamples) {
	int i;
	pthread_t threads[nrOfThreads];
	thread_info_t infos[nrOfThreads];
	int isThreadRunning[nrOfThreads];
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

	benchmark->nrOfThreads = nrOfThreads;

	for (i = 0; i < nrOfThreads ; i += 1) {
		pthread_join(threads[i], NULL);
		elapsedTime += ((infos[i].end.tv_sec - infos[i].begin.tv_sec) * 1000000) + (infos[i].end.tv_usec - infos[i].begin.tv_usec);
		result.skips += infos[i].skips;
	}

	benchmark->nrOfThreads = 0;

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
	int i = 0;

	gettimeofday(&info->begin, NULL);
	while (i < info->nrOfSamples) {
		if (info->benchmark->state == BENCHMARK_STATE_RUNNING ) {
			//TODO race condition?? or not because of the mutex on changing the state
			__sync_add_and_fetch(&info->benchmark->threadsRunning, 1);
			benchmark_runSamples(info, &i, &info->benchmark->state);
			__sync_sub_and_fetch(&info->benchmark->threadsRunning, 1);
		} else {
			usleep(WAIT_TIME);
		}
	}
	gettimeofday(&info->end, NULL);

}

static void benchmark_runSamples(thread_info_t *info, int *i, volatile benchmark_state_t *state) {
	int nrOfSamples = info->nrOfSamples;
	unsigned int result = info->result;
	math_service_pt math = info->benchmark->math;

	for (; *i < nrOfSamples && *state == BENCHMARK_STATE_RUNNING; *i += 1) {
		result = math->calc(math->handle, result, *i);
	}

	info->result = result;
}

char * benchmark_getName(benchmark_pt benchmark) {
	return (char *)BENCHMARK_NAME;
}

static void benchmark_continue(benchmark_pt benchmark) {
	benchmark->state = BENCHMARK_STATE_RUNNING;
	unsigned long waitTime = 0;
	while (benchmark->threadsRunning < benchmark->nrOfThreads) {
		usleep(WAIT_TIME);
		waitTime += WAIT_TIME;
		if (waitTime > 1000 * 1000 * 2) {
			printf("still waiting to stop, running threads are %i\n",
					benchmark->threadsRunning);
		}
	}
}

static void benchmark_interrupt(benchmark_pt benchmark) {
	int i = 0;
	unsigned long waitTime = 0;
	if (benchmark->state == BENCHMARK_STATE_RUNNING) {
		benchmark->state = BENCHMARK_STATE_INTERRUPTED;
		while (benchmark->threadsRunning > 0) {
			usleep(WAIT_TIME);
			waitTime += WAIT_TIME;
			if (waitTime > 1000 * 1000 * 2) {
				printf("still waiting to stop, running threads are %i\n",
						benchmark->threadsRunning);
			}
		}
	}
}

celix_status_t benchmark_addMathService(benchmark_pt benchmark, math_service_pt mathService) {
	pthread_mutex_lock(&benchmark->mutex);
	benchmark_interrupt(benchmark);
	benchmark->math = mathService;
	benchmark_continue(benchmark);
	pthread_mutex_unlock(&benchmark->mutex);
	return CELIX_SUCCESS;
}

celix_status_t benchmark_removeMathService(benchmark_pt benchmark, math_service_pt mathService) {
	pthread_mutex_lock(&benchmark->mutex);
	if (benchmark->math == mathService) {
		benchmark_interrupt(benchmark);
		benchmark->math = NULL;
		benchmark_continue(benchmark);
	}
	pthread_mutex_unlock(&benchmark->mutex);
	return CELIX_SUCCESS;

}

double benchmark_getSampleFactor(benchmark_pt benchmark) {
	return SAMPLE_FACTOR;
}
