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
