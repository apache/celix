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
 *  Created on: Aug 23, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "publisher_private.h"

struct activatorData {
	PUBLISHER_SERVICE ps;
	PUBLISHER pub;
};

void * bundleActivator_create() {
	struct activatorData * data = (struct activatorData *) malloc(sizeof(*data));
	return data;
}

void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	struct activatorData * data = (struct activatorData *) userData;
	data->ps = malloc(sizeof(*(data->ps)));
	data->pub = malloc(sizeof(*(data->pub)));
	data->ps->invoke = publisher_invoke;
	data->ps->publisher = data->pub;

	bundleContext_registerService(context, PUBLISHER_NAME, data->ps, NULL);
}

void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	struct activatorData * data = (struct activatorData *) userData;
	data->ps->publisher = NULL;
	data->ps->invoke = NULL;
	free(data->pub);
	free(data->ps);
	data->pub = NULL;
	data->ps = NULL;
}

void bundleActivator_destroy(void * userData) {
	free(userData);
}
