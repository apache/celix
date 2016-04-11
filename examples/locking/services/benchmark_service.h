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
