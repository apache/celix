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
 * consumer.c
 *
 *  \date       Feb 3, 2014
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include "consumer.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#include <urcu.h>

#include "math_service.h"
#include "frequency_service.h"

#define FREELIST_LENGTH 16

typedef union service_counter service_counter_t;

union service_counter {
	volatile struct {
		volatile u_int32_t counter; //TODO FIXME assuming little endian!!
		volatile u_int32_t position;
		math_service_pt math; // not accesible by raw
	} info; //TODO rename data
	volatile u_int64_t data; //TODO rename raw
};

struct consumer {
	math_service_pt math;
	frequency_service_pt frequencyService;
	locking_type_t currentLockingType;
	pthread_mutex_t mutex;
	pthread_rwlock_t rw_lock;
	service_counter_t *counters[FREELIST_LENGTH];
	service_counter_t *current;
};

typedef struct run_info {
	consumer_pt consumer;
	volatile locking_type_t type;
	int nrOfsamples;
	int result;
	uint skips;
	uint updateCounter;
	struct timeval begin;
	struct timeval end;
} run_info_t;

static void * consumer_reference_run(run_info_t *info);
static void * consumer_no_locking_run(run_info_t *info);
static void * consumer_mutex_run(run_info_t *info);
static void * consumer_rcu_run(run_info_t *info);
static void * consumer_reference_counting_run(run_info_t *info);
static void * consumer_rw_lock_run(run_info_t *info);

static int consumer_reference_calc(int arg1, int arg2);

celix_status_t consumer_create(consumer_pt *result) {
	consumer_pt consumer = malloc(sizeof(*consumer));
	consumer->math = NULL;
	consumer->frequencyService = NULL;
	consumer->currentLockingType=LOCKING_TYPE_NO_LOCKING;


	service_counter_t *new = malloc(sizeof(service_counter_t));
	new->info.position = 0;
	new->info.counter = 0;
	new->info.math = NULL;

	int i;
	for (i = 0; i < FREELIST_LENGTH; i+=1) {
		consumer->counters[i] = NULL;
	}
	consumer->current = new;
	consumer->counters[0] = new;

	pthread_mutex_init(&consumer->mutex, NULL);
	pthread_rwlock_init(&consumer->rw_lock, NULL);

	rcu_init();

	(*result) = consumer;
	return CELIX_SUCCESS;
}

celix_status_t consumer_destroy(consumer_pt consumer) {
	pthread_mutex_destroy(&consumer->mutex);
	pthread_rwlock_destroy(&consumer->rw_lock);
	free(consumer);
	return CELIX_SUCCESS;
}

void consumer_setFrequencyService(consumer_pt consumer, frequency_service_pt freqServ) {
	consumer->frequencyService=freqServ;
}

void consumer_runBenchmark(consumer_pt consumer, locking_type_t type, int nrOfThreads, int nrOfSamples) {
	pthread_t threads[nrOfThreads];
	run_info_t info[nrOfThreads];
	int elapsedTime, skips, counter;
	double callTime, callFreq, updateFreq;
	int i;

	consumer->currentLockingType=type;
	usleep(1000);

	//init
	for (i = 0; i< nrOfThreads; i += 1) {
		info[i].consumer = consumer;
		info[i].nrOfsamples=nrOfSamples;
		info[i].result = rand();
		info[i].skips = 0;
		info[i].updateCounter = 0;
	}
	elapsedTime = 0;
	skips = 0;

	//start threads
	info->consumer->frequencyService->resetCounter(info->consumer->frequencyService->handler);
	for (i = 0; i < nrOfThreads; i += 1) {
		if (type == LOCKING_TYPE_NO_LOCKING) {
			pthread_create(&threads[i], NULL, (void *)consumer_no_locking_run, &info[i]);
		} else if (type == LOCKING_TYPE_MUTEX) {
			pthread_create(&threads[i], NULL, (void *)consumer_mutex_run, &info[i]);
		} else if (type == LOCKING_TYPE_REFERENCE) {
			pthread_create(&threads[i], NULL, (void *)consumer_reference_run, &info[i]);
		} else if (type == LOCKING_TYPE_RCU) {
			pthread_create(&threads[i], NULL, (void *)consumer_rcu_run, &info[i]);
		} else if (type == LOCKING_TYPE_REFERENCE_COUNTER) {
			pthread_create(&threads[i], NULL, (void *)consumer_reference_counting_run, &info[i]);
		} else if (type == LOCKING_TYPE_RW_LOCK) {
			pthread_create(&threads[i], NULL, (void *)consumer_rw_lock_run, &info[i]);
		} else {
			printf ("unknown type\n");
			return;
		}
	}

	//join and print result

	for (i = 0; i < nrOfThreads; i +=1 ) {
		pthread_join(threads[i], NULL);
		elapsedTime += ((info[i].end.tv_sec - info[i].begin.tv_sec) * 1000000) + (info[i].end.tv_usec - info[i].begin.tv_usec);
		skips += info[i].skips;
		counter += info[i].updateCounter;
	}
	counter = info->consumer->frequencyService->getCounter(info->consumer->frequencyService->handler);
	callTime = ((double)elapsedTime * 1000) / (nrOfSamples * nrOfThreads);
	callFreq = ((double)(nrOfSamples * nrOfThreads) / elapsedTime);
	updateFreq = ((double)counter * 1000000) / elapsedTime;
	printf("| threads %5i | ", nrOfThreads);
	printf("average call time: % 10.2f nanoseconds | ", callTime);
	printf("frequency calls is % 10.5f MHz | ", callFreq);
	printf("update freq ~ % 8.2f Hz | ", updateFreq);
	printf("\n");

	if (skips > 0) {
		printf("WARNING skips is %i\n", skips);
	}
}

celix_status_t consumer_addMathService(consumer_pt consumer, math_service_pt mathService) {
	if (consumer->currentLockingType == LOCKING_TYPE_MUTEX) {
		pthread_mutex_lock(&consumer->mutex);
		consumer->math = mathService;
		pthread_mutex_unlock(&consumer->mutex);
	} else if (consumer->currentLockingType == LOCKING_TYPE_RCU) {
		consumer->math = mathService;
		synchronize_rcu();
	} else if (consumer->currentLockingType == LOCKING_TYPE_RW_LOCK) {
		pthread_rwlock_wrlock(&consumer->rw_lock);
		consumer->math = mathService;
		pthread_rwlock_unlock(&consumer->rw_lock);
	} else { //no locking
		consumer->math = mathService;
	}

	//always update for reference counter
//	service_counter_t *new = malloc(sizeof(service_counter_t));
//	new->info.position = 0;
//	new->info.counter = 0;
//	new->info.math = mathService;
//	int found = false;
//	int pos;
//	for (pos = 0; !found && pos < FREELIST_LENGTH; pos += 1) {
//		found = __sync_bool_compare_and_swap(&(consumer->counters[pos]), NULL, new);
//		if (found) {
//			new->info.position = pos;
//			break;
//		}
//	}
//
//	if (!found) {
//		printf("Cannot find free spot!!!!, will use 0\n");
//		consumer->counters[0] = new;
//	}
//
//	int changed = false;
//	service_counter_t *old;
//	while (!changed) {
//		old = consumer->current;
//		changed = __sync_bool_compare_and_swap(&consumer->current, old, new);
//	}
//
//	while (old->info.counter != 0) {usleep(10);}
//	consumer->counters[old->info.position] = NULL;
//	free(old);

	return CELIX_SUCCESS;
}

celix_status_t consumer_removeMathService(consumer_pt consumer, math_service_pt mathService) {
	if (consumer->currentLockingType == LOCKING_TYPE_NO_LOCKING) {
		__sync_val_compare_and_swap(&consumer->math, mathService, NULL);
	} else if (consumer->currentLockingType == LOCKING_TYPE_MUTEX) {
		pthread_mutex_lock(&consumer->mutex);
		if (consumer->math == mathService) {
			consumer->math = NULL;
		}
		pthread_mutex_unlock(&consumer->mutex);
	} else if (consumer->currentLockingType == LOCKING_TYPE_RCU) {
		uatomic_cmpxchg(&consumer->math, mathService, NULL);
	} else if (consumer->currentLockingType == LOCKING_TYPE_REFERENCE_COUNTER) {
		//TODO DONT KNOWN IGNORE FOR NOW
	}
	return CELIX_SUCCESS;
}

static void * consumer_reference_run(run_info_t *info) {
	int i;

	gettimeofday(&info->begin, NULL);
	for (i = 0; i < info->nrOfsamples; i += 1) {
		info->result = consumer_reference_calc(info->result, i);
	}
	gettimeofday(&info->end, NULL);
	return NULL;
}

static void * consumer_no_locking_run(run_info_t *info) {
	int i;

	gettimeofday(&info->begin, NULL);
	for (i = 0; i < info->nrOfsamples; i += 1) {
		if (info->consumer->math != NULL) {
				info->result = info->consumer->math->calc(info->result, i);
		} else {
				info->skips +=1; //should not happen
		}
	}
	gettimeofday(&info->end, NULL);
	return NULL;
}

static void * consumer_mutex_run(run_info_t *info) {
	int i;

	gettimeofday(&info->begin, NULL);
	for (i = 0; i < info->nrOfsamples; i += 1) {
		pthread_mutex_lock(&info->consumer->mutex);
		if (info->consumer->math != NULL) {
			info->result = info->consumer->math->calc(info->result, i);
		} else {
			info->skips += 1; //should not happen
		}
		pthread_mutex_unlock(&info->consumer->mutex);
	}
	gettimeofday(&info->end, NULL);
	return NULL;
}

static void * consumer_rw_lock_run(run_info_t *info) {
	int i;
	consumer_pt cons = info->consumer;
	int result = info->result;
	pthread_rwlock_t *lock = &cons->rw_lock;
	int nrOfsamples = info->nrOfsamples;

	gettimeofday(&info->begin, NULL);
	for (i = 0; i < nrOfsamples; i += 1) {
		pthread_rwlock_rdlock(lock);
		if (cons->math != NULL) {
			result = cons->math->calc(result, i);
		} else {
			info->skips += 1; //should not happen
		}
		pthread_rwlock_unlock(lock);
	}
	gettimeofday(&info->end, NULL);
	info->result = result;
	return NULL;
}

static void * consumer_rcu_run(run_info_t *info) {
	rcu_register_thread();
	int i;
	math_service_pt service;

	gettimeofday(&info->begin, NULL);
	for (i = 0; i < info->nrOfsamples; i += 1) {
		rcu_read_lock();
		if (info->consumer->math != NULL) {
			info->result = info->consumer->math->calc(info->result, i);
		} else {
			info->skips +=1; //should not happen
		}
		rcu_read_unlock();
	}
	gettimeofday(&info->end, NULL);
	rcu_unregister_thread();
	return NULL;
}

static void * consumer_reference_counting_run(run_info_t *info) {
	int i;
	service_counter_t posAndCount;

	gettimeofday(&info->begin, NULL);
	for (i = 0; i < info->nrOfsamples; i += 1) {
		posAndCount.data = __sync_add_and_fetch((u_int64_t *)&info->consumer->current->data, 1);
		volatile service_counter_t *serv = (volatile void *)info->consumer->counters[posAndCount.info.position];
		if (serv->info.math != NULL) {
			info->result = serv->info.math->calc(info->result, i);
		} else {
			info->skips += 1;
		}
		__sync_sub_and_fetch((u_int64_t *)&serv->data, -1);

		//not service_counter will not be deleted...but can we still find it??? is info->consumer->serviceCounter still te same?
		//change write to swap compare and then only changing the pointer is the counter is null?? not possible.. can compare counter , but not change pointer

		//IDEA create a list with service_counter based on a id (number) this number is 32bit long and put a counter + id in a 64bit value.
		//use this value to atomic increment and return value and use the id to retrieve the actual pointer. the value can be stored in the heap.
		//A list with id is used to retrieve a pointer to the service. If the value is null the slot is available this can be check with
		//compare_and_swap while looping through the list. The list can be extended when the end is reached and then a next list pointer can
		//be used. This can also be a linked list and the limitation is the max 32bit uint value (of 16bits for 32bit platforms).
	}

	gettimeofday(&info->end, NULL);
	return NULL;
}

//NOTE: copy implementation of the math_service->calc function, for reference.
static int consumer_reference_calc(int arg1, int arg2) {
	return  arg1 * arg2 + arg2;
}
