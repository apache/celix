/*
 * mutex_benchmark.c
 *
 *  Created on: Feb 13, 2014
 *      Author: dl436
 */

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "benchmark.h"

static const char * const BENCHMARK_NAME = "MUTEX";
static const double SAMPLE_FACTOR = 1;

struct benchmark {
	pthread_mutex_t mutex;
	math_service_pt math;
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

celix_status_t benchmark_create(benchmark_pt *benchmark) {
	(*benchmark) = malloc(sizeof(struct benchmark));
	(*benchmark)->math = NULL;
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

	result.averageCallTimeInNanoseconds = ((double)elapsedTime * 1000) / (nrOfSamples * nrOfThreads);
	result.callFrequencyInMhz = ((double)(nrOfSamples * nrOfThreads) / elapsedTime);
	result.nrOfThreads = nrOfThreads;
	result.nrOfsamples = nrOfSamples;

	return result;
}

static void benchmark_thread(thread_info_t *info) {
	int i;

	gettimeofday(&info->begin, NULL);
	for (i = 0; i < info->nrOfSamples; i += 1) {
		pthread_mutex_lock(&info->benchmark->mutex);
		if (info->benchmark->math != NULL) {
			info->result = info->benchmark->math->calc(info->benchmark->math->handle, info->result, i);
		} else {
			info->skips += 1; //should not happen
		}
		pthread_mutex_unlock(&info->benchmark->mutex);
	}
	gettimeofday(&info->end, NULL);

}

char * benchmark_getName(benchmark_pt benchmark) {
	return (char *)BENCHMARK_NAME;
}

celix_status_t benchmark_addMathService(benchmark_pt benchmark, math_service_pt mathService) {
	pthread_mutex_lock(&benchmark->mutex);
	benchmark->math = mathService;
	pthread_mutex_unlock(&benchmark->mutex);
	return CELIX_SUCCESS;
}

celix_status_t benchmark_removeMathService(benchmark_pt benchmark, math_service_pt mathService) {
	pthread_mutex_lock(&benchmark->mutex);
	if (benchmark->math == mathService) {
		benchmark->math = NULL;
	}
	pthread_mutex_unlock(&benchmark->mutex);
	return CELIX_SUCCESS;

}

double benchmark_getSampleFactor(benchmark_pt benchmark) {
	return SAMPLE_FACTOR;
}
