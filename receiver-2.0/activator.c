/*
 * activator.c
 *
 *  Created on: Apr 20, 2010
 *      Author: dk489
 */
#include <stdlib.h>

#include "dependency_manager.h"
#include "dependency_activator_base.h"
#include "service.h"

#include "bundle_activator.h"
#include "bundle_context.h"
#include "serviceTest.h"
#include "serviceTest_private.h"
#include "service_component.h"
#include "service_component_private.h"

SERVICE_REGISTRATION reg;
SERVICE_REGISTRATION reg2;

SERVICE_TEST m_test;

SERVICE m_service;

void * dm_create() {
	return NULL;
}

void dm_init(void * userData, BUNDLE_CONTEXT context, DEPENDENCY_MANAGER manager) {
	m_test = (SERVICE_TEST) malloc(sizeof(*m_test));

	HASHTABLE props = createProperties();
	setProperty(props, "test", "test");
	//setProperty(props, "a", "b");
	m_test->handle = serviceTest_construct();
	m_test->doo = doo;

	m_service = dependencyActivatorBase_createService(manager);
	serviceComponent_setInterface(m_service, SERVICE_TEST_NAME, props);
	serviceComponent_setImplementation(m_service, m_test);

	dependencyManager_add(manager, m_service);
}

void dm_destroy(void * userData, BUNDLE_CONTEXT context, DEPENDENCY_MANAGER manager) {
	dependencyManager_remove(manager, m_service);
}

void service_init(void * userData) {

}
void service_start(void * userData) {

}
void service_stop(void * userData) {

}
void service_destroy(void * userData) {

}

//void bundleActivator_start(BUNDLE_CONTEXT context) {
//	m_test = (SERVICE_TEST) malloc(sizeof(*m_test));
//	HASHTABLE props = createProperties();
//	setProperty(props, "test", "test");
//	//setProperty(props, "a", "b");
//	m_test->handle = serviceTest_construct();
//	m_test->doo = doo;
//
//	reg = register_service(context, SERVICE_TEST_NAME, m_test, props);
//	//reg2 = register_service(context, SERVICE_TEST_NAME, test, NULL);
//}

//void bundleActivator_stop(BUNDLE_CONTEXT context) {
//	serviceRegistration_unregister(reg);
//	serviceTest_destruct(m_test->handle);
//	free(m_test);
//}
