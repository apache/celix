#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#include "array_list.h"

#include "benchmark.h"
#include "math_service.h"

static const char * const BENCHMARK_NAME = "MODIFIED_BIT";
static const double SAMPLE_FACTOR = 1;
static const useconds_t SLEEP_TIME = 10;

typedef struct thread_info {
	benchmark_pt benchmark;
	unsigned int result;
	struct timeval begin;
	struct timeval end;
	int skips;
	bool isModified;
    bool isUpdated;
} thread_info_t;

struct benchmark {
    pthread_mutex_t mutex;
    math_service_pt math;
    int nrOfSamples;
    int nrOfThreads;
    thread_info_t *threads;
};

static void benchmark_thread(thread_info_t *info);

celix_status_t benchmark_create(benchmark_pt *benchmark) {
	(*benchmark) = malloc(sizeof(struct benchmark));
	(*benchmark)->math = NULL;
	pthread_mutex_init(&(*benchmark)->mutex, NULL);
    return CELIX_SUCCESS;
}

celix_status_t benchmark_destroy(benchmark_pt benchmark) {
    //free threads array
	free(benchmark);
    return CELIX_SUCCESS;
}

benchmark_result_t benchmark_run(benchmark_pt benchmark, int nrOfThreads, int nrOfSamples) {
	int i;
	pthread_t threads[nrOfThreads];
	thread_info_t infos[nrOfThreads];
	benchmark_result_t result;
	unsigned long elapsedTime = 0;

	benchmark->threads = infos;
	benchmark->nrOfSamples = nrOfSamples;
    benchmark->nrOfThreads = nrOfThreads;

	result.skips =0;
    pthread_mutex_lock(&benchmark->mutex);
	for (i = 0 ; i < nrOfThreads ; i += 1) {
		infos[i].benchmark = benchmark;
		infos[i].skips = 0;
		infos[i].result = rand();
		infos[i].isUpdated = false;
		infos[i].isModified = false;
		pthread_create(&threads[i], NULL, (void *)benchmark_thread,  &infos[i]);
	}
    pthread_mutex_unlock(&benchmark->mutex);

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
	int i = 0;

    math_service_pt local = info->benchmark->math;
	int nrOfSamples = info->benchmark->nrOfSamples;
	
	gettimeofday(&info->begin, NULL);
	while (i < nrOfSamples) {
		if (!info->isModified) {
            if (local != NULL) {
                info->result = local->calc(local->handle, info->result, i);
            } else {
                info->skips += 1; //should not happen
            }
            i += 1;
        } else {
			local = info->benchmark->math;
			info->isModified = false;
		}
	}
	gettimeofday(&info->end, NULL);

}

char * benchmark_getName(benchmark_pt benchmark) {
	return (char *)BENCHMARK_NAME;
}

celix_status_t benchmark_addMathService(benchmark_pt benchmark, math_service_pt mathService) {
	int i;

	pthread_mutex_lock(&benchmark->mutex);
	benchmark->math = mathService;
	pthread_mutex_unlock(&benchmark->mutex);

	//inform threads to update servicd
	for (i = 0 ; i < benchmark->nrOfSamples ; i += 1) {
		benchmark->threads[i].isModified = true;
	}	

	//Wait till al thread are not in a modified state, e.g. update to date mathService and not using the old service
	for (i = 0; i < benchmark->nrOfThreads ; i += 1) {
		if (benchmark->threads[i].isModified) {
			usleep(SLEEP_TIME);
		}
	}

	return CELIX_SUCCESS;
}

celix_status_t benchmark_removeMathService(benchmark_pt benchmark, math_service_pt mathService) {
	pthread_mutex_lock(&benchmark->mutex);
	if (benchmark->math == mathService) {
		benchmark->math = NULL;
	} 
	pthread_mutex_unlock(&benchmark->mutex);

	//inform threads to update servicd
    int i;
	for (i = 0 ; i < benchmark->nrOfThreads ; i += 1) {
		benchmark->threads[i].isModified = true;
	}	

	//Wait till al thread are not in a modified state, e.g. update to date mathService and not using the old service
	for (i = 0; i < benchmark->nrOfThreads ; i += 1) {
		if (benchmark->threads[i].isModified) {
			usleep(SLEEP_TIME);
		}
	}

	
	return CELIX_SUCCESS;
}

double benchmark_getSampleFactor(benchmark_pt benchmark) {
	return SAMPLE_FACTOR;
}
