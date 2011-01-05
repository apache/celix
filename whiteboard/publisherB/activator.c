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

void * bundleActivator_create() {
	return NULL;
}

void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	PUBLISHER_SERVICE ps = malloc(sizeof(*ps));
	PUBLISHER pub = malloc(sizeof(*pub));
	ps->invoke = publisher_invoke;
	ps->publisher = pub;

	bundleContext_registerService(context, PUBLISHER_NAME, ps, NULL);
}

void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {

}

void bundleActivator_destroy(void * userData) {

}
