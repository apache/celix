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
 * example_service.h
 *
 *  \date       Oct 5, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef EXAMPLE_SERVICE_H_
#define EXAMPLE_SERVICE_H_

#define EXAMPLE_SERVICE "example"

typedef struct example *example_pt;

typedef struct example_service *example_service_pt;

struct example_service {
	example_pt example;
	celix_status_t (*add)(example_pt example, double a, double b, double *result);
	celix_status_t (*sub)(example_pt example, double a, double b, double *result);
	celix_status_t (*sqrt)(example_pt example, double a, double *result);
};

#endif /* EXAMPLE_SERVICE_H_ */
