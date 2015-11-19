/*
 * benchmark_service.h
 *
 *  Created on: Feb 13, 2014
 *      Author: dl436
 */

#ifndef BENCHMARK_SERVICE_H_
#define BENCHMARK_SERVICE_H_

#include "benchmark_result.h"

typedef struct benchmark_service *benchmark_service_pt;

typedef struct benchmark_handler *benchmark_handler_pt; //ADT

#define BENCHMARK_SERVICE_NAME "benchmark_service"

struct benchmark_service {
	benchmark_handler_pt handler;

	benchmark_result_t (*run)(benchmark_handler_pt handler, int nrOfThreads, int nrOfSamples);
	char * (*name)(benchmark_handler_pt handler);
	double (*getSampleFactor)(benchmark_handler_pt benchmark);
};

#endif /* BENCHMARK_SERVICE_H_ */
