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
 * calculator_service.h
 *
 *  \date       Oct 5, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CALCULATOR_SERVICE_H_
#define CALCULATOR_SERVICE_H_

#define CALCULATOR_SERVICE "org.example.api.Calculator"

typedef struct calculator *calculator_pt;

typedef struct calculator_service *calculator_service_pt;

struct calculator_service {
	calculator_pt calculator;
	celix_status_t (*add)(calculator_pt calculator, double a, double b, double *result);
	celix_status_t (*sub)(calculator_pt calculator, double a, double b, double *result);
	celix_status_t (*sqrt)(calculator_pt calculator, double a, double *result);
};


/*
 * interface calculator_service {
 * 	double add(double a, double b);
 * 	double sub(double a, double b);
 * 	double sqrt(double a);
 * }
 */

#endif /* CALCULATOR_SERVICE_H_ */
