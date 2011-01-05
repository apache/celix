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
 * activator.c
 *
 *  Created on: Aug 20, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>

#include "bundle_activator.h"

struct userData {
	char * word;
};

void * bundleActivator_create() {
	struct userData * data = malloc(sizeof(*data));
	data->word = "World";
	return data;
}

void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	struct userData * data = (struct userData *) userData;
	printf("Hello %s\n", data->word);

}

void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	struct userData * data = (struct userData *) userData;
	printf("Goodbye %s\n", data->word);
}

void bundleActivator_destroy(void * userData) {

}
