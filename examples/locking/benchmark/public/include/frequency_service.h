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
 * frequence_service.h
 *
 *  \date       Feb 4, 2014
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

//TODO change to math provider service ???

#ifndef FREQUENCY_SERVICE_H
#define FREQUENCY_SERVICE_H

#define FREQUENCY_SERVICE_NAME "frequency_service"

typedef struct frequency_hander frequence_handler_t;

struct frequency_service {
	frequence_handler_t *handle;
	void (*setFrequency)(frequence_handler_t *handle, int freq);
	void (*resetCounter)(frequence_handler_t *handle);
	uint (*getCounter)(frequence_handler_t *handle);
	void (*setBenchmarkName)(frequence_handler_t *handle, char *name);
	void (*setNrOfThreads)(frequence_handler_t *handle, uint nrOfThreads);
};

typedef struct frequency_service * frequency_service_pt;

#endif /* FREQUENCY_SERVICE_H */
