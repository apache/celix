/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <gtest/gtest.h>
#include "TopologyManagerTestSuiteBaseClass.h"

class TopologyManagerTestSuite : public TopologyManagerTestSuiteBaseClass {
public:
    TopologyManagerTestSuite() = default;

    ~TopologyManagerTestSuite() = default;
};

TEST_F(TopologyManagerTestSuite, ExportService1Test) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef, void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx) {
        (void)ctx;
        //first add the rsa, then the epl and then the exported service
        auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerAdded(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerRemoved(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(TopologyManagerTestSuite, ExportService2Test) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef, void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx) {
        (void)ctx;
        //first add the rsa, then the exported service and then the epl
        auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerAdded(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerRemoved(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(TopologyManagerTestSuite, ExportService3Test) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef, void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx) {
        (void)ctx;
        //first add the exported service, then the rsa and then the epl
        auto status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerAdded(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = topologyManager_endpointListenerRemoved(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(TopologyManagerTestSuite, ExportServiceWithDynamicIP1Test) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef, void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx) {
        (void)ctx;
        //first add the rsa, then the epl and then the exported service
        auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerAdded(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerRemoved(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, true);
}

TEST_F(TopologyManagerTestSuite, ExportServiceWithDynamicIP2Test) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef, void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx) {
        (void)ctx;
        //first add the rsa, then the exported service and then the epl
        auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerAdded(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerRemoved(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, true);
}

TEST_F(TopologyManagerTestSuite, ExportServiceWithDynamicIP3Test) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef, void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx) {
        (void)ctx;
        //first add the exported service, then the rsa and then the epl
        auto status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerAdded(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = topologyManager_endpointListenerRemoved(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, true);
}

TEST_F(TopologyManagerTestSuite, ExportMutilServiceWithDynamicIPTest) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef, void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx) {
        //first add the rsa, then the exported service and then the epl
        auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerAdded(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);

        struct TmTestService {
            void* handle;
        } exportedSvc2{};
        auto exportedSvcProps = celix_properties_create();
        celix_properties_set(exportedSvcProps, "service.exported.interfaces", "*");
        auto exportedSvcId = celix_bundleContext_registerService(ctx, &exportedSvc2, "tmTestService2", exportedSvcProps);
        EXPECT_TRUE(exportedSvcId > 0);
        service_reference_pt svc2Ref{};
        status = bundleContext_getServiceReference(ctx, "tmTestService2", &svc2Ref);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_addExportedService(tm, svc2Ref, &exportedSvc2);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_removeExportedService(tm, svc2Ref, &exportedSvc2);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = bundleContext_ungetServiceReference(ctx, svc2Ref);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_bundleContext_unregisterService(ctx, exportedSvcId);

        status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerRemoved(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, true);
}

TEST_F(TopologyManagerTestSuite, ExportServiceFailed1Test) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef, void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx) {
        (void)ctx;
        //first add the rsa, then the exported service and then the epl
        auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
        status = topologyManager_endpointListenerAdded(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = topologyManager_endpointListenerRemoved(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, false,
    [](remote_service_admin_t* admin, char* serviceId, celix_properties_t* properties, celix_array_list_t** registrations) -> celix_status_t {
        (void)admin;
        (void)properties;
        (void)serviceId;
        (void)registrations;
        return CELIX_BUNDLE_EXCEPTION;
    },
    [](void *handle, endpoint_description_t *endpoint, char *matchedFilter) -> celix_status_t {
        (void)handle;
        (void)endpoint;
        (void)matchedFilter;
        ADD_FAILURE() << "Should not be called";
        return CELIX_SUCCESS;
    });
}

TEST_F(TopologyManagerTestSuite, ExportServiceFailed2Test) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef, void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx) {
        (void)ctx;
        //first add the exported service, then the rsa and then the epl
        auto status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerAdded(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = topologyManager_endpointListenerRemoved(tm, eplSvcRef, eplSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, false,
    [](remote_service_admin_t* admin, char* serviceId, celix_properties_t* properties, celix_array_list_t** registrations) -> celix_status_t {
      (void)admin;
      (void)properties;
      (void)serviceId;
      (void)registrations;
      return CELIX_BUNDLE_EXCEPTION;
    },
    [](void *handle, endpoint_description_t *endpoint, char *matchedFilter) -> celix_status_t {
      (void)handle;
      (void)endpoint;
      (void)matchedFilter;
      ADD_FAILURE() << "Should not be called";
      return CELIX_SUCCESS;
    });
}

TEST_F(TopologyManagerTestSuite, ExportEmptyRegistrationListTest) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef, void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx) {
          (void)ctx;
          //first add the rsa, then the exported service and then the epl
          auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
          status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
          status = topologyManager_endpointListenerAdded(tm, eplSvcRef, eplSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);

          status = topologyManager_endpointListenerRemoved(tm, eplSvcRef, eplSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
          status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
          status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
      }, true,
      [](remote_service_admin_t* admin, char* serviceId, celix_properties_t* properties, celix_array_list_t** registrations) -> celix_status_t {
          (void)admin;
          (void)properties;
          (void)serviceId;
          *registrations = celix_arrayList_createPointerArray();
          return CELIX_SUCCESS;
      },
      [](void *handle, endpoint_description_t *endpoint, char *matchedFilter) -> celix_status_t {
          (void)handle;
          (void)endpoint;
          (void)matchedFilter;
          ADD_FAILURE() << "Should not be called";
          return CELIX_SUCCESS;
      });
}

TEST_F(TopologyManagerTestSuite, DynamicIpEndpointRsaPortNotSpecifiedTest) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef, void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx) {
          (void)ctx;
          //first add the rsa, then the exported service and then the epl
          auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
          status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
          status = topologyManager_endpointListenerAdded(tm, eplSvcRef, eplSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);

          status = topologyManager_endpointListenerRemoved(tm, eplSvcRef, eplSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
          status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
          status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
      }, true,
      [](remote_service_admin_t* admin, char* serviceId, celix_properties_t* properties, celix_array_list_t** registrations) -> celix_status_t {
          (void)properties;
          celix_array_list_t* references{};
          service_reference_pt reference{};
          char filter[256];
          snprintf(filter, 256, "(%s=%s)", (char *) CELIX_FRAMEWORK_SERVICE_ID, serviceId);
          auto status = bundleContext_getServiceReferences(admin->ctx, nullptr, filter, &references);
          EXPECT_EQ(CELIX_SUCCESS, status);
          reference = (service_reference_pt)celix_arrayList_get(references, 0);
          EXPECT_TRUE(reference != nullptr);
          auto exportReg = (export_registration_t*)malloc(sizeof(export_registration_t));
          auto endpointProps = celix_properties_create();
          unsigned int size = 0;
          char **keys;
          serviceReference_getPropertyKeys(reference, &keys, &size);
          for (unsigned int i = 0; i < size; i++) {
              char *key = keys[i];
              const char *value{};
              if (serviceReference_getProperty(reference, key, &value) == CELIX_SUCCESS
                  && strcmp(key, (char*) CELIX_RSA_SERVICE_EXPORTED_INTERFACES) != 0
                  && strcmp(key, (char*) CELIX_RSA_SERVICE_EXPORTED_CONFIGS) != 0) {
                  celix_properties_set(endpointProps, key, value);
              }
          }
          const char *fwUuid = celix_bundleContext_getProperty(admin->ctx, CELIX_FRAMEWORK_UUID, nullptr);
          celix_properties_set(endpointProps, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
          celix_properties_set(endpointProps, CELIX_RSA_ENDPOINT_SERVICE_ID, serviceId);
          celix_properties_set(endpointProps, CELIX_RSA_ENDPOINT_ID, "319bddfa-0252-4654-a3bd-298354d30207");
          celix_properties_set(endpointProps, CELIX_RSA_SERVICE_IMPORTED, "true");
          celix_properties_set(endpointProps, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, "tm_test_config_type");
          status = endpointDescription_create(endpointProps, &exportReg->exportReference.endpoint);
          EXPECT_EQ(CELIX_SUCCESS, status);
          exportReg->exportReference.reference = reference;
          *registrations = celix_arrayList_createPointerArray();
          celix_arrayList_add(*registrations, exportReg);

          bundleContext_ungetServiceReference(admin->ctx, reference);
          free(keys);
          celix_arrayList_destroy(references);
          return CELIX_SUCCESS;
      },
      [](void *handle, endpoint_description_t *endpoint, char *matchedFilter) -> celix_status_t {
          (void)handle;
          (void)endpoint;
          (void)matchedFilter;
          ADD_FAILURE() << "Should not be called";
          return CELIX_SUCCESS;
      });
}

TEST_F(TopologyManagerTestSuite, DynamicIpRsaNetworkInterfaceNotSpecifiedTest) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef, void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx) {
          (void)ctx;
          unsetenv("CELIX_RSA_INTERFACES_OF_PORT_" TEST_RSA_PORT);
          //first add the rsa, then the exported service and then the epl
          auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
          status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
          status = topologyManager_endpointListenerAdded(tm, eplSvcRef, eplSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);

          status = topologyManager_endpointListenerRemoved(tm, eplSvcRef, eplSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
          status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
          status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
          EXPECT_EQ(CELIX_SUCCESS, status);
      }, true,nullptr,
      [](void *handle, endpoint_description_t *endpoint, char *matchedFilter) -> celix_status_t {
          (void)handle;
          (void)endpoint;
          (void)matchedFilter;
          ADD_FAILURE() << "Should not be called";
          return CELIX_SUCCESS;
      });
}

TEST_F(TopologyManagerTestSuite, EndpointListenerNotSupportInterfaceSpecificEndpointsTest) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef, void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx) {
        (void)ctx;
        (void)eplSvc;
        (void)eplSvcRef;
        //first add the rsa, then the epl and then the exported service
        auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);

        endpoint_listener_t epl{};
        epl.handle = nullptr;
        epl.endpointAdded = [](void *handle, endpoint_description_t *endpoint, char *matchedFilter) -> celix_status_t {
            (void)handle;
            (void)endpoint;
            (void)matchedFilter;
            ADD_FAILURE() << "Should not be called";
            return CELIX_SUCCESS;
        };
        epl.endpointRemoved = [](void *handle, endpoint_description_t *endpoint, char *matchedFilter) -> celix_status_t {
            (void)handle;
            (void)endpoint;
            (void)matchedFilter;
            return CELIX_SUCCESS;
        };
        const char *fwUuid = celix_bundleContext_getProperty(ctx, CELIX_FRAMEWORK_UUID, nullptr);
        char scope[256] = {0};
        (void)snprintf(scope, sizeof(scope), "(&(%s=*)(%s=%s))", CELIX_FRAMEWORK_SERVICE_NAME,
                       CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
        auto elpProps = celix_properties_create();
        celix_properties_set(elpProps, CELIX_RSA_ENDPOINT_LISTENER_SCOPE, scope);
        auto eplId = celix_bundleContext_registerService(ctx, &epl, CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME, elpProps);
        EXPECT_TRUE(eplId > 0);
        char filter[256];
        snprintf(filter, 256, "(%s=%ld)", (char *) CELIX_FRAMEWORK_SERVICE_ID, eplId);
        celix_array_list_t* references{};
        status = bundleContext_getServiceReferences(ctx, nullptr, filter, &references);
        EXPECT_EQ(CELIX_SUCCESS, status);
        auto eplRef = (service_reference_pt)celix_arrayList_get(references, 0);
        EXPECT_TRUE(eplRef != nullptr);
        status = topologyManager_endpointListenerAdded(tm, eplRef, &epl);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_endpointListenerRemoved(tm, eplRef, &epl);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = bundleContext_ungetServiceReference(ctx, eplRef);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_arrayList_destroy(references);
        celix_bundleContext_unregisterService(ctx, eplId);
    }, true);
}

TEST_F(TopologyManagerTestSuite, ImportService1Test) {
    TestImportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, endpoint_description_t *importEndpoint) {
        //first add the rsa and then the import endpoint
        auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_addImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = topologyManager_removeImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(TopologyManagerTestSuite, ImportService2Test) {
    TestImportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, endpoint_description_t *importEndpoint) {
        //first add the import endpoint and then the rsa
        auto status = topologyManager_addImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_removeImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}