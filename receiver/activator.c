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
