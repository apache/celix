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
 *  Created on: Apr 20, 2010
 *      Author: dk489
 */
#include <stdlib.h>
#include "bundle_activator.h"
#include "bundle_context.h"
#include "serviceTest.h"
#include "serviceTest_private.h"

SERVICE_REGISTRATION reg;
SERVICE_REGISTRATION reg2;

SERVICE_TEST m_test;

void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	m_test = (SERVICE_TEST) malloc(sizeof(*m_test));
	HASHTABLE props = createProperties();
	setProperty(props, "test", "test");
	//setProperty(props, "a", "b");
	m_test->handle = serviceTest_construct();
	m_test->doo = doo;

	reg = bundleContext_registerService(context, SERVICE_TEST_NAME, m_test, props);
	//reg2 = register_service(context, SERVICE_TEST_NAME, test, NULL);
}

void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	serviceRegistration_unregister(reg);
	serviceTest_destruct(m_test->handle);
	free(m_test);
}

void bundleActivator_destroy(void * userData) {

}
