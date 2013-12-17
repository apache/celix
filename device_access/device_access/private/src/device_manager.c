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
 * device_manager.c
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <constants.h>
#include <apr_general.h>

#include "device_manager.h"
#include "driver_locator.h"
#include "driver_matcher.h"
#include "driver_attributes.h"
#include "driver_loader.h"
#include "driver.h"
#include "device.h"

#include <bundle.h>
#include <module.h>
#include <array_list.h>
#include <service_registry.h>
#include <service_reference.h>

struct device_manager {
	apr_pool_t *pool;
	bundle_context_pt context;
	hash_map_pt devices;
	hash_map_pt drivers;
	array_list_pt locators;
	driver_selector_service_pt selector;
};

static celix_status_t deviceManager_attachAlgorithm(device_manager_pt manager, service_reference_pt ref, void *service);
static celix_status_t deviceManager_getIdleDevices(device_manager_pt manager, apr_pool_t *pool, array_list_pt *idleDevices);
static celix_status_t deviceManager_isDriverBundle(device_manager_pt manager, bundle_pt bundle, bool *isDriver);

celix_status_t deviceManager_create(apr_pool_t *pool, bundle_context_pt context, device_manager_pt *manager) {
	celix_status_t status = CELIX_SUCCESS;

	*manager = apr_palloc(pool, sizeof(**manager));
	if (!*manager) {
		status = CELIX_ENOMEM;
	} else {
		(*manager)->pool = pool;
		(*manager)->context = context;
		(*manager)->devices = NULL;
		(*manager)->drivers = NULL;
		(*manager)->locators = NULL;
		(*manager)->selector = NULL;

		(*manager)->devices = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
		(*manager)->drivers = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
		status = arrayList_create(&(*manager)->locators);
	}

	printf("DEVICE_MANAGER: Initialized\n");
	return status;
}

celix_status_t deviceManager_destroy(device_manager_pt manager) {
	celix_status_t status = CELIX_SUCCESS;

	printf("DEVICE_MANAGER: Stop\n");
	hashMap_destroy(manager->devices, false, false);
	hashMap_destroy(manager->drivers, false, false);
	arrayList_destroy(manager->locators);

	return status;
}

celix_status_t deviceManager_selectorAdded(void * handle, service_reference_pt ref, void * service) {
	printf("DEVICE_MANAGER: Add selector\n");
	device_manager_pt manager = handle;
	manager->selector = (driver_selector_service_pt) service;
	return CELIX_SUCCESS;
}

celix_status_t deviceManager_selectorModified(void * handle, service_reference_pt ref, void * service) {
	printf("DEVICE_MANAGER: Modify selector\n");
	return CELIX_SUCCESS;
}

celix_status_t deviceManager_selectorRemoved(void * handle, service_reference_pt ref, void * service) {
	printf("DEVICE_MANAGER: Remove selector\n");
	device_manager_pt manager = handle;
	manager->selector = NULL;
	return CELIX_SUCCESS;
}

celix_status_t deviceManager_locatorAdded(void * handle, service_reference_pt ref, void * service) {
	printf("DEVICE_MANAGER: Add locator\n");
	device_manager_pt manager = handle;
	arrayList_add(manager->locators, service);
	return CELIX_SUCCESS;
}

celix_status_t deviceManager_locatorModified(void * handle, service_reference_pt ref, void * service) {
	printf("DEVICE_MANAGER: Modify locator\n");
	return CELIX_SUCCESS;
}

celix_status_t deviceManager_locatorRemoved(void * handle, service_reference_pt ref, void * service) {
	printf("DEVICE_MANAGER: Remove locator\n");
	device_manager_pt manager = handle;
	arrayList_removeElement(manager->locators, service);
	return CELIX_SUCCESS;
}

celix_status_t deviceManager_deviceAdded(void * handle, service_reference_pt ref, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DEVICE_MANAGER: Add device\n");
	device_manager_pt manager = handle;

	status = deviceManager_attachAlgorithm(manager, ref, service);

	return status;
}

static celix_status_t deviceManager_attachAlgorithm(device_manager_pt manager, service_reference_pt ref, void *service) {
	celix_status_t status = CELIX_SUCCESS;

	apr_pool_t *attachPool = NULL;
	apr_status_t aprStatus = apr_pool_create(&attachPool, manager->pool);
	if (aprStatus != APR_SUCCESS) {
		status = CELIX_ILLEGAL_STATE;
	} else {
		driver_loader_pt loader = NULL;
		status = driverLoader_create(attachPool, manager->context, &loader);
		if (status == CELIX_SUCCESS) {
			array_list_pt included = NULL;
			array_list_pt excluded = NULL;

			array_list_pt driverIds = NULL;

			hashMap_put(manager->devices, ref, service);

			status = arrayList_create(&included);
			if (status == CELIX_SUCCESS) {
				status = arrayList_create(&excluded);
				if (status == CELIX_SUCCESS) {
					service_registration_pt registration = NULL;
					status = serviceReference_getServiceRegistration(ref, &registration);
					if (status == CELIX_SUCCESS) {
						properties_pt properties = NULL;
						status = serviceRegistration_getProperties(registration, &properties);
						if (status == CELIX_SUCCESS) {
							status = driverLoader_findDrivers(loader, attachPool, manager->locators, properties, &driverIds);
							if (status == CELIX_SUCCESS) {
								hash_map_iterator_pt iter = hashMapIterator_create(manager->drivers);
								while (hashMapIterator_hasNext(iter)) {
									driver_attributes_pt driverAttributes = hashMapIterator_nextValue(iter);
									arrayList_add(included, driverAttributes);

									// Each driver that already is installed can be removed from the list
									char *id = NULL;
									celix_status_t substatus = driverAttributes_getDriverId(driverAttributes, &id);
									if (substatus == CELIX_SUCCESS) {
										// arrayList_removeElement(driverIds, id);
										array_list_iterator_pt idsIter = arrayListIterator_create(driverIds);
										while (arrayListIterator_hasNext(idsIter)) {
											char *value = arrayListIterator_next(idsIter);
											if (strcmp(value, id) == 0) {
												arrayListIterator_remove(idsIter);
											}
										}
										arrayListIterator_destroy(idsIter);
									} else {
										// Ignore
									}
								}
								hashMapIterator_destroy(iter);

								status = deviceManager_matchAttachDriver(manager, attachPool, loader, driverIds, included, excluded, service, ref);
								arrayList_destroy(driverIds);
							}
						}
					}
					arrayList_destroy(excluded);
				}
				arrayList_destroy(included);
			}

		}
		apr_pool_destroy(attachPool);
	}
	return status;
}

celix_status_t deviceManager_matchAttachDriver(device_manager_pt manager, apr_pool_t *attachPool, driver_loader_pt loader,
		array_list_pt driverIds, array_list_pt included, array_list_pt excluded, void *service, service_reference_pt reference) {
	celix_status_t status = CELIX_SUCCESS;

	array_list_pt references = NULL;

	int i;
	for (i = 0; i < arrayList_size(excluded); i++) {
		void *exclude = arrayList_get(excluded, i);
		arrayList_removeElement(included, exclude);
	}

	for (i = 0; i < arrayList_size(driverIds); i++) {
		char *id = arrayList_get(driverIds, i);
		printf("DEVICE_MANAGER: Driver found: %s\n", id);
	}

	status = driverLoader_loadDrivers(loader, attachPool, manager->locators, driverIds, &references);
	if (status == CELIX_SUCCESS) {
		for (i = 0; i < arrayList_size(references); i++) {
			service_reference_pt reference = arrayList_get(references, i);
			driver_attributes_pt attributes = hashMap_get(manager->drivers, reference);
			if (attributes != NULL) {
				arrayList_add(included, attributes);
			}
		}

		driver_matcher_pt matcher = NULL;
		status = driverMatcher_create(attachPool, manager->context, &matcher);
		if (status == CELIX_SUCCESS) {
			for (i = 0; i < arrayList_size(included); i++) {
				driver_attributes_pt attributes = arrayList_get(included, i);

				int match = 0;
				celix_status_t substatus = driverAttributes_match(attributes, reference, &match);
				if (substatus == CELIX_SUCCESS) {
					printf("DEVICE_MANAGER: Found match: %d\n", match);
					if (match <= OSGI_DEVICEACCESS_DEVICE_MATCH_NONE) {
						continue;
					}
					driverMatcher_add(matcher, match, attributes);
				} else {
					// Ignore
				}
			}

			match_pt match = NULL;
			status = driverMatcher_getBestMatch(matcher, attachPool, reference, &match);
			if (status == CELIX_SUCCESS) {
				if (match == NULL) {
					status = deviceManager_noDriverFound(manager, service, reference);
				} else {
					service_registration_pt registration = NULL;
					status = serviceReference_getServiceRegistration(match->reference, &registration);
					if (status == CELIX_SUCCESS) {
						properties_pt properties = NULL;
						status = serviceRegistration_getProperties(registration, &properties);
						if (status == CELIX_SUCCESS) {
							char *driverId = properties_get(properties, (char *) OSGI_DEVICEACCESS_DRIVER_ID);
							driver_attributes_pt finalAttributes = hashMap_get(manager->drivers, match->reference);
							if (finalAttributes == NULL) {
								status = deviceManager_noDriverFound(manager, service, reference);
							} else {
								char *newDriverId = NULL;
								status = driverAttributes_attach(finalAttributes, reference, &newDriverId);
								if (status == CELIX_SUCCESS) {
									if (newDriverId != NULL) {
										array_list_pt ids = NULL;
										arrayList_create(&ids);
										arrayList_add(ids, newDriverId);
										arrayList_add(excluded, finalAttributes);
										status = deviceManager_matchAttachDriver(manager, attachPool, loader,
												ids, included, excluded, service, reference);
									} else {
										// Attached, unload unused drivers
										status = driverLoader_unloadDrivers(loader, finalAttributes);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (references != NULL) {
		arrayList_destroy(references);
	}

	return status;
}

celix_status_t deviceManager_noDriverFound(device_manager_pt manager, void *service, service_reference_pt reference) {
	celix_status_t status = CELIX_SUCCESS;
	service_registration_pt registration = NULL;
	status = serviceReference_getServiceRegistration(reference, &registration);
	if (status == CELIX_SUCCESS) {
		properties_pt properties = NULL;
		status = serviceRegistration_getProperties(registration, &properties);
		if (status == CELIX_SUCCESS) {
			char *objectClass = properties_get(properties, (char *) OSGI_FRAMEWORK_OBJECTCLASS);
			if (strcmp(objectClass, OSGI_DEVICEACCESS_DRIVER_SERVICE_NAME) == 0) {
				device_service_pt device = service;
				status = device->noDriverFound(device->device);
			}
		}
	}
	return status;
}

celix_status_t deviceManager_deviceModified(void * handle, service_reference_pt ref, void * service) {
	printf("DEVICE_MANAGER: Modify device\n");
	// #TODO the device properties could be changed
	//device_manager_pt manager = handle;
	//hashMap_put(manager->devices, ref, service);
	return CELIX_SUCCESS;
}

celix_status_t deviceManager_deviceRemoved(void * handle, service_reference_pt ref, void * service) {
	printf("DEVICE_MANAGER: Remove device\n");
	device_manager_pt manager = handle;
	hashMap_remove(manager->devices, ref);
	return CELIX_SUCCESS;
}

celix_status_t deviceManager_driverAdded(void * handle, service_reference_pt ref, void * service) {
	celix_status_t status = CELIX_SUCCESS;

	printf("DEVICE_MANAGER: Add driver\n");
	device_manager_pt manager = handle;
	apr_pool_t *pool = NULL;
	driver_attributes_pt attributes = NULL;

	status = apr_pool_create(&pool, manager->pool);
	if (status == CELIX_SUCCESS) {
		status = driverAttributes_create(pool, ref, service, &attributes);
		if (status == CELIX_SUCCESS) {
			hashMap_put(manager->drivers, ref, attributes);
		}
	}
	return status;
}

celix_status_t deviceManager_driverModified(void * handle, service_reference_pt ref, void * service) {
	printf("DEVICE_MANAGER: Modify driver\n");
	// #TODO the driver properties could be changed?
	return CELIX_SUCCESS;
}

celix_status_t deviceManager_driverRemoved(void * handle, service_reference_pt ref, void * service) {
	celix_status_t status = CELIX_SUCCESS;

	printf("DEVICE_MANAGER: Remove driver\n");
	device_manager_pt manager = handle;
	hashMap_remove(manager->drivers, ref);

	apr_pool_t *idleCheckPool;
	apr_status_t aprStatus = apr_pool_create(&idleCheckPool, manager->pool);
	if (aprStatus != APR_SUCCESS) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		array_list_pt idleDevices = NULL;
		status = deviceManager_getIdleDevices(manager, idleCheckPool, &idleDevices);
		if (status == CELIX_SUCCESS) {
			int i;
			for (i = 0; i < arrayList_size(idleDevices); i++) {
				celix_status_t forStatus = CELIX_SUCCESS;
				service_reference_pt ref = arrayList_get(idleDevices, i);
				char *bsn = NULL;
				bundle_pt bundle = NULL;
				forStatus = serviceReference_getBundle(ref, &bundle);
				if (forStatus == CELIX_SUCCESS) {
					module_pt module = NULL;
					forStatus = bundle_getCurrentModule(bundle, &module);
					if (forStatus == CELIX_SUCCESS) {
						forStatus = module_getSymbolicName(module, &bsn);
						if (forStatus == CELIX_SUCCESS) {
							printf("DEVICE_MANAGER: IDLE: %s\n", bsn);
							// #TODO attachDriver (idle device)
							// #TODO this can result in a loop?
							//		Locate and install a driver
							//		Let the match fail, the device is idle
							//		The driver is removed, idle check is performed
							//		Attach is tried again
							//		.. loop ..
							void *device = hashMap_get(manager->devices, ref);
							forStatus = deviceManager_attachAlgorithm(manager, ref, device);
						}
					}
				}

				if (forStatus != CELIX_SUCCESS) {
					break; //Got error, stop loop and return status
				}
			}


			hash_map_iterator_pt iter = hashMapIterator_create(manager->drivers);
			while (hashMapIterator_hasNext(iter)) {
				driver_attributes_pt da = hashMapIterator_nextValue(iter);
				//driverAttributes_tryUninstall(da);
			}
			hashMapIterator_destroy(iter);
		}

		if (idleDevices != NULL) {
			arrayList_destroy(idleDevices);
		}
	}

	return status;
}


celix_status_t deviceManager_getIdleDevices(device_manager_pt manager, apr_pool_t *pool, array_list_pt *idleDevices) {
	celix_status_t status = CELIX_SUCCESS;

	status = arrayList_create(idleDevices);
	if (status == CELIX_SUCCESS) {
		hash_map_iterator_pt iter = hashMapIterator_create(manager->devices);
		while (hashMapIterator_hasNext(iter)) {
			celix_status_t substatus = CELIX_SUCCESS;
			service_reference_pt ref = hashMapIterator_nextKey(iter);
			char *bsn = NULL;
			module_pt module = NULL;
			bundle_pt bundle = NULL;
			substatus = serviceReference_getBundle(ref, &bundle);
			if (substatus == CELIX_SUCCESS) {
				substatus = bundle_getCurrentModule(bundle, &module);
				if (substatus == CELIX_SUCCESS) {
					substatus = module_getSymbolicName(module, &bsn);
					if (substatus == CELIX_SUCCESS) {
						printf("DEVICE_MANAGER: Check idle device: %s\n", bsn);
						array_list_pt bundles = NULL;
						substatus = serviceReference_getUsingBundles(ref, pool, &bundles);
						if (substatus == CELIX_SUCCESS) {
							bool inUse = false;
							int i;
							for (i = 0; i < arrayList_size(bundles); i++) {
								bundle_pt bundle = arrayList_get(bundles, i);
								bool isDriver;
								celix_status_t sstatus = deviceManager_isDriverBundle(manager, bundle, &isDriver);
								if (sstatus == CELIX_SUCCESS) {
									if (isDriver) {
										char *bsn = NULL;
										module_pt module = NULL;
										bundle_getCurrentModule(bundle, &module);
										module_getSymbolicName(module, &bsn);

										printf("DEVICE_MANAGER: Not idle, used by driver: %s\n", bsn);

										inUse = true;
										break;
									}
								}
							}

							if (!inUse) {
								arrayList_add(*idleDevices, ref);
							}
						}
					}
				}
			}
		}
		hashMapIterator_destroy(iter);
	}

	return status;
}

//TODO examply for discussion only, remove after discussion
#define DO_IF_SUCCESS(status, call_func) ((status) == CELIX_SUCCESS) ? (call_func) : (status)
celix_status_t deviceManager_getIdleDevices_exmaple(device_manager_pt manager, apr_pool_t *pool, array_list_pt *idleDevices) {
	celix_status_t status = CELIX_SUCCESS;

	status = arrayList_create(idleDevices);
	if (status == CELIX_SUCCESS) {
		hash_map_iterator_pt iter = hashMapIterator_create(manager->devices);
		while (hashMapIterator_hasNext(iter)) {
			celix_status_t substatus = CELIX_SUCCESS;
			service_reference_pt ref = hashMapIterator_nextKey(iter);
			char *bsn = NULL;
			module_pt module = NULL;
			bundle_pt bundle = NULL;
			array_list_pt bundles = NULL;
			substatus = serviceReference_getBundle(ref, &bundle);
			substatus = DO_IF_SUCCESS(substatus, bundle_getCurrentModule(bundle, &module));
			substatus = DO_IF_SUCCESS(substatus, module_getSymbolicName(module, &bsn));
			substatus = DO_IF_SUCCESS(substatus, serviceReference_getUsingBundles(ref, pool, &bundles));

			if (substatus == CELIX_SUCCESS) {
				printf("DEVICE_MANAGER: Check idle device: %s\n", bsn);
				bool inUse = false;
				int i;
				for (i = 0; i < arrayList_size(bundles); i++) {
					bundle_pt bundle = arrayList_get(bundles, i);
					bool isDriver;
					celix_status_t sstatus = deviceManager_isDriverBundle(manager, bundle, &isDriver);
					if (sstatus == CELIX_SUCCESS) {
						if (isDriver) {
							char *bsn = NULL;
							module_pt module = NULL;
							bundle_getCurrentModule(bundle, &module);
							module_getSymbolicName(module, &bsn);

							printf("DEVICE_MANAGER: Not idle, used by driver: %s\n", bsn);

							inUse = true;
							break;
						}
					}
				}

				if (!inUse) {
					arrayList_add(*idleDevices, ref);
				}
			}
		}
	}
	return status;
}

celix_status_t deviceManager_isDriverBundle(device_manager_pt manager, bundle_pt bundle, bool *isDriver) {
	celix_status_t status = CELIX_SUCCESS;
	(*isDriver) = false;

	array_list_pt refs = NULL;
	apr_pool_t *pool = NULL;
	status = bundle_getMemoryPool(bundle, &pool);
	if (status == CELIX_SUCCESS) {
		status = bundle_getRegisteredServices(bundle, pool, &refs);
		if (status == CELIX_SUCCESS) {
			if (refs != NULL) {
				int i;
				for (i = 0; i < arrayList_size(refs); i++) {
					celix_status_t substatus = CELIX_SUCCESS;
					service_reference_pt ref = arrayList_get(refs, i);
					service_registration_pt registration = NULL;
					substatus = serviceReference_getServiceRegistration(ref, &registration);
					if (substatus == CELIX_SUCCESS) {
						properties_pt properties = NULL;
						substatus = serviceRegistration_getProperties(registration, &properties);
						if (substatus == CELIX_SUCCESS) {
							char *object = properties_get(properties, (char *) OSGI_FRAMEWORK_OBJECTCLASS);
																if (strcmp(object, "driver") == 0) {
																	*isDriver = true;
																	break;
																}
						}

					}
				}
				arrayList_destroy(refs);
			}
		}

	}

	return status;
}


celix_status_t deviceManager_getBundleContext(device_manager_pt manager, bundle_context_pt *context) {
	celix_status_t status = CELIX_SUCCESS;
	if (manager->context != NULL) {
		(*context) = manager->context;
	} else {
		status = CELIX_INVALID_BUNDLE_CONTEXT;
	}
	return status;
}



