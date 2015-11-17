/*
 * consumer.h
 *
 *  Created on: Feb 13, 2014
 *      Author: dl436
 */

#ifndef CONSUMER_H_
#define CONSUMER_H_

#include "celix_errno.h"

#include "benchmark_result.h"
#include "math_service.h"

typedef struct benchmark *benchmark_pt; //ADT

celix_status_t benchmark_create(benchmark_pt *benchmark);
celix_status_t benchmark_destroy(benchmark_pt benchmark);

benchmark_result_t benchmark_run(benchmark_pt benchmark, int nrOfThreads, int nrOfSamples);
char * benchmark_getName(benchmark_pt benchmark);
double benchmark_getSampleFactor(benchmark_pt benchmark);

celix_status_t benchmark_addMathService(benchmark_pt benchmark, math_service_pt mathService);
celix_status_t benchmark_removeMathService(benchmark_pt benchmark, math_service_pt mathService);


#endif /* CONSUMER_H_ */
