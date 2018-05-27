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
 * topology_manager_scoped_test.cpp
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <jansson.h>

#include "celix_launcher.h"
#include "constants.h"
#include "endpoint_description.h"
#include "framework.h"
#include "topology_manager.h"
#include "calculator_service.h"
#include "tm_scope.h"
#include "scope.h"
#include "remote_service_admin.h"
#include "remote_constants.h"
#include "disc_mock_service.h"
#include "tst_service.h"

#define JSON_EXPORT_SERVICES  "exportServices"
#define JSON_IMPORT_SERVICES  "importServices"

#define JSON_SERVICE_NAME     "filter"
#define JSON_SERVICE_ZONE     "zone"
#define JSON_SERVICE_KEY1     "key1"
#define JSON_SERVICE_KEY2     "key2"

    static framework_pt framework = NULL;
    static bundle_context_pt context = NULL;

    static service_reference_pt scopeServiceRef = NULL;
    static tm_scope_service_pt tmScopeService = NULL;

    static service_reference_pt calcRef = NULL;
    static calculator_service_pt calc = NULL;

    static service_reference_pt rsaRef = NULL;
    static remote_service_admin_service_pt rsa = NULL;

    static service_reference_pt discRef = NULL;
    static disc_mock_service_pt discMock = NULL;

    static service_reference_pt testRef = NULL;
    static tst_service_pt testImport = NULL;

    static service_reference_pt eplRef = NULL;
    static endpoint_listener_pt eplService = NULL; // actually this is the topology manager


    static void setupFm(void) {
        int rc = 0;
        rc = celixLauncher_launch("config.properties", &framework);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        bundle_pt bundle = NULL;
        rc = framework_getFrameworkBundle(framework, &bundle);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundle_getContext(bundle, &context);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *)OSGI_RSA_REMOTE_SERVICE_ADMIN, &rsaRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK(rsaRef != NULL);

        rc = bundleContext_getService(context, rsaRef, (void **)&rsa);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *)TOPOLOGYMANAGER_SCOPE_SERVICE, &scopeServiceRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK(scopeServiceRef != NULL);

        rc = bundleContext_getService(context, scopeServiceRef, (void **)&tmScopeService);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *)CALCULATOR2_SERVICE, &calcRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK(calcRef != NULL);

        rc = bundleContext_getService(context, calcRef, (void **)&calc);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *)DISC_MOCK_SERVICE_NAME, &discRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK(discRef != NULL);

        rc = bundleContext_getService(context, discRef, (void **)&discMock);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
    }

    static void teardownFm(void) {
        int rc = 0;

        rc = bundleContext_ungetService(context, scopeServiceRef, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,scopeServiceRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetService(context, calcRef, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,calcRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetService(context, rsaRef, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,rsaRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetService(context, discRef, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,discRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        celixLauncher_stop(framework);
        celixLauncher_waitForShutdown(framework);
        celixLauncher_destroy(framework);

        scopeServiceRef = NULL;
        tmScopeService = NULL;
        calcRef = NULL;
        calc = NULL;

        rsaRef = NULL;
        rsa = NULL;
        discRef = NULL;
        discMock = NULL;

        testRef = NULL;
        testImport = NULL;

        eplRef = NULL;
        eplService = NULL;

        context = NULL;
        framework = NULL;
    }

    static void setupFmImport(void) {
        int rc = 0;
        rc = celixLauncher_launch("config_import.properties", &framework);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        bundle_pt bundle = NULL;
        rc = framework_getFrameworkBundle(framework, &bundle);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundle_getContext(bundle, &context);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *)OSGI_RSA_REMOTE_SERVICE_ADMIN, &rsaRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK(rsaRef != NULL);

        rc = bundleContext_getService(context, rsaRef, (void **)&rsa);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *)TOPOLOGYMANAGER_SCOPE_SERVICE, &scopeServiceRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK(scopeServiceRef != NULL);

        rc = bundleContext_getService(context, scopeServiceRef, (void **)&tmScopeService);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *)TST_SERVICE_NAME, &testRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK(testRef != NULL);

        rc = bundleContext_getService(context, testRef, (void **)&testImport);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char*)OSGI_ENDPOINT_LISTENER_SERVICE, &eplRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK(eplRef != NULL);

        rc = bundleContext_getService(context, eplRef, (void **)&eplService);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
    }

    static void teardownFmImport(void) {
        int rc = 0;

        rc = bundleContext_ungetService(context, rsaRef, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,rsaRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetService(context, scopeServiceRef, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,scopeServiceRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetService(context, testRef, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,testRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetService(context, eplRef, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,eplRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        celixLauncher_stop(framework);
        celixLauncher_waitForShutdown(framework);
        celixLauncher_destroy(framework);

        scopeServiceRef = NULL;
        tmScopeService = NULL;
        calcRef = NULL;
        calc = NULL;

        rsaRef = NULL;
        rsa = NULL;
        discRef = NULL;
        discMock = NULL;

        testRef = NULL;
        testImport = NULL;

        eplRef = NULL;
        eplService = NULL;

        context = NULL;
        framework = NULL;
    }


    /// \TEST_CASE_ID{1}
    /// \TEST_CASE_TITLE{Test register scope service}
    /// \TEST_CASE_REQ{REQ-1}
    /// \TEST_CASE_DESC Checks if 3 bundles are installed after the framework setup
    static void testBundles(void) {
    	printf("Begin: %s\n", __func__);
            array_list_pt bundles = NULL;

            int rc = bundleContext_getBundles(context, &bundles);
            CHECK_EQUAL(0, rc);
            CHECK_EQUAL(5, arrayList_size(bundles)); //framework, scopeService & calc & rsa

            /*
            int size = arrayList_size(bundles);
            int i;
            for (i = 0; i < size; i += 1) {
                bundle_pt bundle = NULL;
                module_pt module = NULL;
                char *name = NULL;

                bundle = (bundle_pt) arrayList_get(bundles, i);
                bundle_getCurrentModule(bundle, &module);
                module_getSymbolicName(module, &name);
                printf("got bundle with symbolic name '%s'", name);
            }*/

            arrayList_destroy(bundles);
        	printf("End: %s\n", __func__);
    }

    static void scopeInit(const char *fileName, int *nr_exported, int *nr_imported)
    {
    	celix_status_t	status = CELIX_SUCCESS;
    	celix_status_t  added;

        json_t *js_root;
        json_error_t error;
        properties_pt properties;

        *nr_exported = 0;
        *nr_imported = 0;
        js_root = json_load_file(fileName, 0, &error);
        if (js_root != NULL)
        {
            json_t *js_exportServices = json_object_get(js_root, JSON_EXPORT_SERVICES);
            json_t *js_importServices = json_object_get(js_root, JSON_IMPORT_SERVICES);

            if (js_exportServices != NULL)  {
                 if (json_is_array(js_exportServices)) {
                     int i = 0;
                     int size = json_array_size(js_exportServices);

                     for (; i < size; ++i) {
                         json_t* js_service = json_array_get(js_exportServices, i);

                         if (json_is_object(js_service)) {
                        	 json_t* js_filter = json_object_get(js_service, JSON_SERVICE_NAME);
                        	 json_t* js_serviceZone = json_object_get(js_service, JSON_SERVICE_ZONE);
                        	 json_t* js_key1 = json_object_get(js_service, JSON_SERVICE_KEY1);
                        	 json_t* js_key2 = json_object_get(js_service, JSON_SERVICE_KEY2);

                             properties=properties_create();
                             if (js_serviceZone != NULL) {
                        	     properties_set(properties, (char*)JSON_SERVICE_ZONE,
                        		    	                    (char*)json_string_value(js_serviceZone));
			     }
                             if (js_key1 != NULL) {
                        	     properties_set(properties, (char*)JSON_SERVICE_KEY1,
                        		    	 	 	  	 	    (char*)json_string_value(js_key1));
			     }
                             if (js_key2 != NULL) {
                                 properties_set(properties, (char*)JSON_SERVICE_KEY2,
                                    	 	 	  	 	    (char*)json_string_value(js_key2));
			     }

                        	 added = tmScopeService->addExportScope(tmScopeService->handle, (char*)json_string_value(js_filter), properties);
                             if (added == CELIX_SUCCESS) {
                            	 (*nr_exported)++;
                             }
                         }
                     }
                 }
             }

             if (js_importServices != NULL)  {
                 if (json_is_array(js_importServices)) {
                     int i = 0;
                     int size = json_array_size(js_importServices);

                     for (; i < size; ++i) {
                         json_t* js_service = json_array_get(js_importServices, i);

                         if (json_is_object(js_service)) {
                        	 json_t* js_filter = json_object_get(js_service, JSON_SERVICE_NAME);

                        	 added = tmScopeService->addImportScope(tmScopeService->handle, (char*)json_string_value(js_filter));
                             if (added == CELIX_SUCCESS) {
                            	 (*nr_imported)++;
                             }
                         }
                     }
                 }
             }


        	json_decref(js_root);
        }
        else
        {
        	printf("File error: %s\n", error.text);
        	printf("File error: source %s\n", error.source);
        	printf("File error: line %d position %d\n", error.line, error.position);
        	status = CELIX_FILE_IO_EXCEPTION;
        }
        CHECK_EQUAL(CELIX_SUCCESS, status);
    }

    /// \TEST_CASE_ID{2}
    /// \TEST_CASE_TITLE{Test scope initialisation}
    /// \TEST_CASE_REQ{REQ-2}
    /// \TEST_CASE_DESC Checks if scopes can be added, but not twice
    static void testScope(void) {
    	int nr_exported;
    	int nr_imported;
    	array_list_pt epList;

    	printf("\nBegin: %s\n", __func__);
        scopeInit("scope.json", &nr_exported, &nr_imported);
        CHECK_EQUAL(1, nr_exported);
        CHECK_EQUAL(0, nr_imported);

        discMock->getEPDescriptors(discMock->handle, &epList);
        // We export two services: Calculator and Calculator2, but only 1 has DFI bundle info
        CHECK_EQUAL(1, arrayList_size(epList));
        for (unsigned int i = 0; i < arrayList_size(epList); i++) {
        	endpoint_description_pt ep = (endpoint_description_pt) arrayList_get(epList, i);
        	properties_pt props = ep->properties;
        	hash_map_entry_pt entry = hashMap_getEntry(props, (void*)"key2");
        	char* value = (char*) hashMapEntry_getValue(entry);
        	STRCMP_EQUAL("inaetics", value);
        	/*
        	printf("Service: %s ", ep->service);
        	hash_map_iterator_pt iter = hashMapIterator_create(props);
        	while (hashMapIterator_hasNext(iter)) {
        		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
        		printf("%s - %s\n", (char*)hashMapEntry_getKey(entry),
        						   (char*)hashMapEntry_getValue(entry));
        	}
        	printf("\n");
        	hashMapIterator_destroy(iter);
        	*/
        }
        printf("End: %s\n", __func__);
    }

    /// \TEST_CASE_ID{3}
    /// \TEST_CASE_TITLE{Test scope initialisation}
    /// \TEST_CASE_REQ{REQ-3}
    /// \TEST_CASE_DESC Checks if scopes can be added, but not twice
    static void testScope2(void) {
    	int nr_exported;
    	int nr_imported;
    	array_list_pt epList;
    	printf("\nBegin: %s\n", __func__);
        scopeInit("scope2.json", &nr_exported, &nr_imported);
        CHECK_EQUAL(2, nr_exported);
        CHECK_EQUAL(1, nr_imported);
        discMock->getEPDescriptors(discMock->handle, &epList);
        // We export two services: Calculator and Calculator2, but only 1 has DFI bundle info
        CHECK_EQUAL(1, arrayList_size(epList));
        for (unsigned int i = 0; i < arrayList_size(epList); i++) {
        	endpoint_description_pt ep = (endpoint_description_pt) arrayList_get(epList, i);
        	properties_pt props = ep->properties;
        	hash_map_entry_pt entry = hashMap_getEntry(props, (void*)"key2");
        	char* value = (char*) hashMapEntry_getValue(entry);
        	STRCMP_EQUAL("inaetics", value);
        }
        printf("End: %s\n", __func__);
    }

    /// \TEST_CASE_ID{4}
    /// \TEST_CASE_TITLE{Test scope initialisation}
    /// \TEST_CASE_REQ{REQ-4}
    /// \TEST_CASE_DESC Checks if scopes can be added, but not twice
    static void testScope3(void) {
    	int nr_exported;
    	int nr_imported;
    	array_list_pt epList;
    	printf("\nBegin: %s\n", __func__);
        scopeInit("scope3.json", &nr_exported, &nr_imported);
        CHECK_EQUAL(2, nr_exported);
        discMock->getEPDescriptors(discMock->handle, &epList);
        // We export two services: Calculator and Calculator2, but only 1 has DFI bundle info
        CHECK_EQUAL(1, arrayList_size(epList));
        for (unsigned int i = 0; i < arrayList_size(epList); i++) {
        	endpoint_description_pt ep = (endpoint_description_pt) arrayList_get(epList, i);
        	properties_pt props = ep->properties;
        	hash_map_entry_pt entry = hashMap_getEntry(props, (void *)"key2");
        	char* value = (char*) hashMapEntry_getValue(entry);
        	STRCMP_EQUAL("inaetics", value);
        }
        printf("End: %s\n", __func__);
    }

    /// \TEST_CASE_ID{5}
    /// \TEST_CASE_TITLE{Test scope initialisation}
    /// \TEST_CASE_REQ{REQ-5}
    /// \TEST_CASE_DESC Invalid input file, two partly matching filters with same key name
    /*
    static void testScope4(void) {
    	int nr_exported;
    	int nr_imported;
    	array_list_pt epList;
    	printf("\nBegin: %s\n", __func__);
        scopeInit("scope4.json", &nr_exported, &nr_imported);
        CHECK_EQUAL(2, nr_exported);
        discMock->getEPDescriptors(discMock->handle, &epList);
        // We export two services: Calculator and Calculator2, but only 1 has DFI bundle info
        CHECK_EQUAL(1, arrayList_size(epList));
        for (unsigned int i = 0; i < arrayList_size(epList); i++) {
        	endpoint_description_pt ep = (endpoint_description_pt) arrayList_get(epList, i);
        	properties_pt props = ep->properties;
        	hash_map_entry_pt entry = hashMap_getEntry(props, (void*)"zone");
        	char* value = (char*) hashMapEntry_getValue(entry);
        	STRCMP_EQUAL("inaetics", value);
        	CHECK_TRUE((entry == NULL));
        }
        printf("End: %s\n", __func__);
    }*/


    /// \TEST_CASE_ID{6}
    /// \TEST_CASE_TITLE{Test import scope}
    /// \TEST_CASE_REQ{REQ-3}
    /// \TEST_CASE_DESC Checks if import succeeds if there is no import scope defined
    static void testImportScope(void) {
    	int nr_exported;
    	int nr_imported;
    	printf("\nBegin: %s\n", __func__);

    	scopeInit("scope.json", &nr_exported, &nr_imported);
    	CHECK_EQUAL(0, nr_imported);
        int rc = 0;

        endpoint_description_pt endpoint = NULL;

        properties_pt props = properties_create();
        properties_set(props, (char *)OSGI_RSA_ENDPOINT_SERVICE_ID, (char *)"42");
        properties_set(props, (char *)OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, (char *)"eec5404d-51d0-47ef-8d86-c825a8beda42");
        properties_set(props, (char *)OSGI_RSA_ENDPOINT_ID, (char *)"eec5404d-51d0-47ef-8d86-c825a8beda42-42");
        properties_set(props, (char *)OSGI_FRAMEWORK_OBJECTCLASS,(char *)"org.apache.celix.test.MyBundle");
        properties_set(props, (char *)"service.version",(char *)"1.0.0"); //TODO find out standard in osgi spec
        properties_set(props, (char *)"zone", (char *)"thales");

        rc = endpointDescription_create(props, &endpoint);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = eplService->endpointAdded(eplService->handle, endpoint, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        bool imported = testImport->IsImported(testImport);
        CHECK_EQUAL(true, imported);

        rc = eplService->endpointRemoved(eplService->handle, endpoint, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = endpointDescription_destroy(endpoint);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        printf("*****After importService\n");
        printf("End: %s\n", __func__);
    }

    /// \TEST_CASE_ID{7}
    /// \TEST_CASE_TITLE{Test import scope}
    /// \TEST_CASE_REQ{REQ-3}
    /// \TEST_CASE_DESC Checks if import succeeds if there is a matching import scope defined
    static void testImportScopeMatch(void) {
        int nr_exported;
        int nr_imported;
        printf("\nBegin: %s\n", __func__);

        scopeInit("scope2.json", &nr_exported, &nr_imported);
        CHECK_EQUAL(1, nr_imported);
        int rc = 0;

        endpoint_description_pt endpoint = NULL;

        properties_pt props = properties_create();
        properties_set(props, (char *)OSGI_RSA_ENDPOINT_SERVICE_ID, (char *)"42");
        properties_set(props, (char *)OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, (char *)"eec5404d-51d0-47ef-8d86-c825a8beda42");
        properties_set(props, (char *)OSGI_RSA_ENDPOINT_ID, (char *)"eec5404d-51d0-47ef-8d86-c825a8beda42-42");
        properties_set(props, (char *)OSGI_FRAMEWORK_OBJECTCLASS,(char *)"org.apache.celix.test.MyBundle");
        properties_set(props, (char *)"service.version",(char *)"1.0.0"); //TODO find out standard in osgi spec
        properties_set(props, (char *)"zone", (char *)"thales");

        rc = endpointDescription_create(props, &endpoint);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = eplService->endpointAdded(eplService->handle, endpoint, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        bool imported = testImport->IsImported(testImport);
        CHECK_EQUAL(true, imported);

        rc = eplService->endpointRemoved(eplService->handle, endpoint, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = endpointDescription_destroy(endpoint);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        printf("End: %s\n", __func__);
    }


    /// \TEST_CASE_ID{8}
    /// \TEST_CASE_TITLE{Test import scope block}
    /// \TEST_CASE_REQ{REQ-3}
    /// \TEST_CASE_DESC Checks if import fails with non matching import scope defined
    static void testImportScopeFail(void) {
        int nr_exported;
        int nr_imported;
        printf("\nBegin: %s\n", __func__);

        scopeInit("scope3.json", &nr_exported, &nr_imported);
        CHECK_EQUAL(1, nr_imported);
        int rc = 0;

        endpoint_description_pt endpoint = NULL;

        properties_pt props = properties_create();
        properties_set(props, (char *)OSGI_RSA_ENDPOINT_SERVICE_ID, (char *)"42");
        properties_set(props, (char *)OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, (char *)"eec5404d-51d0-47ef-8d86-c825a8beda42");
        properties_set(props, (char *)OSGI_RSA_ENDPOINT_ID, (char *)"eec5404d-51d0-47ef-8d86-c825a8beda42-42");
        properties_set(props, (char *)OSGI_FRAMEWORK_OBJECTCLASS,(char *)"org.apache.celix.test.MyBundle");
        properties_set(props, (char *)"service.version",(char *)"1.0.0"); //TODO find out standard in osgi spec
        properties_set(props, (char *)"zone", (char *)"thales");

        rc = endpointDescription_create(props, &endpoint);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = eplService->endpointAdded(eplService->handle, endpoint, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        bool imported = testImport->IsImported(testImport);
        CHECK_EQUAL(false, imported);

        rc = eplService->endpointRemoved(eplService->handle, endpoint, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = endpointDescription_destroy(endpoint);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        printf("End: %s\n", __func__);
    }

    /// \TEST_CASE_ID{8}
    /// \TEST_CASE_TITLE{Test import scope block}
    /// \TEST_CASE_REQ{REQ-3}
    /// \TEST_CASE_DESC Checks if import fails with non matching import scope defined
    static void testImportScopeMultiple(void) {
        int nr_exported;
        int nr_imported;
        printf("\nBegin: %s\n", __func__);

        scopeInit("scope4.json", &nr_exported, &nr_imported);
        CHECK_EQUAL(2, nr_imported);
        int rc = 0;

        endpoint_description_pt endpoint = NULL;

        properties_pt props = properties_create();
        properties_set(props, (char *)OSGI_RSA_ENDPOINT_SERVICE_ID, (char *)"42");
        properties_set(props, (char *)OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, (char *)"eec5404d-51d0-47ef-8d86-c825a8beda42");
        properties_set(props, (char *)OSGI_RSA_ENDPOINT_ID, (char *)"eec5404d-51d0-47ef-8d86-c825a8beda42-42");
        properties_set(props, (char *)OSGI_FRAMEWORK_OBJECTCLASS,(char *)"org.apache.celix.test.MyBundle");
        properties_set(props, (char *)"service.version",(char *)"1.0.0"); //TODO find out standard in osgi spec
        properties_set(props, (char *)"zone", (char *)"thales");

        rc = endpointDescription_create(props, &endpoint);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = eplService->endpointAdded(eplService->handle, endpoint, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        bool imported = testImport->IsImported(testImport);
        CHECK_EQUAL(true, imported);

        rc = eplService->endpointRemoved(eplService->handle, endpoint, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = endpointDescription_destroy(endpoint);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        printf("End: %s\n", __func__);
    }

}

TEST_GROUP(topology_manager_scoped_export) {
	void setup(void) {
		setupFm();
	}

	void teardown() {
		teardownFm();
	}
};

TEST_GROUP(topology_manager_scoped_import) {
	void setup(void) {
		setupFmImport();
	}

	void teardown() {
		teardownFmImport();
	}
};

// Test9
TEST(topology_manager_scoped_import, scope_import_multiple) {
	testImportScopeMultiple();

}

// Test8
TEST(topology_manager_scoped_import, scope_import_fail) {
	testImportScopeFail();

}

// Test7
TEST(topology_manager_scoped_import, scope_import_match) {
	testImportScopeMatch();

}

// Test6
TEST(topology_manager_scoped_import, scope_import) {
	testImportScope();

}

// Test5
/*
TODO: NYI
TEST(topology_manager_scoped_export, scope_init4) {
	testScope4();
}
*/

// Test4
TEST(topology_manager_scoped_export, scope_init3) {
	testScope3();

}

// Test3
TEST(topology_manager_scoped_export, scope_init2) {
	testScope2();

}

// Test2
TEST(topology_manager_scoped_export, scope_init) {
	testScope();

}

// Test1
TEST(topology_manager_scoped_export, init_test) {
    testBundles();
}
