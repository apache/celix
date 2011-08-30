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
 * headers.h
 *
 *  Created on: Mar 25, 2010
 *      Author: alexanderb
 */

#ifndef HEADERS_H_
#define HEADERS_H_

#include <stdio.h>
#include <dirent.h>
#include <pthread.h>

#include <apr_general.h>
#include <apr_thread_proc.h>
#include <apr_thread_cond.h>
#include <apr_thread_mutex.h>
#include <apr_portable.h>

#include "array_list.h"
#include "properties.h"
#include "linkedlist.h"
#include "version.h"
#include "version_range.h"
#include "manifest.h"
#include "hash_map.h"
#include "bundle_archive.h"
#include "bundle_state.h"
#include "bundle_cache.h"

#if defined(__GNUC__)
#define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#else
#define ATTRIBUTE_UNUSED
#endif


#define celix_log(msg) printf("%s\n\tat %s(%s:%d)\n", msg, __func__, __FILE__, __LINE__);

struct framework {
	struct bundle * bundle;
	HASH_MAP installedBundleMap;
	HASH_MAP installRequestMap;
	ARRAY_LIST serviceListeners;

	long nextBundleId;
	struct serviceRegistry * registry;
	BUNDLE_CACHE cache;

	apr_thread_cond_t *shutdownGate;
	apr_thread_cond_t *condition;

	apr_thread_mutex_t *installRequestLock;
	apr_thread_mutex_t *mutex;
	apr_thread_mutex_t *bundleLock;

	pthread_t globalLockThread;
	ARRAY_LIST globalLockWaitersList;
	int globalLockCount;

	bool interrupted;
	bool shutdown;

	apr_pool_t *mp;
};

typedef struct framework * FRAMEWORK;

typedef struct bundleContext * BUNDLE_CONTEXT;

typedef struct activator * ACTIVATOR;

typedef struct module * MODULE;

typedef struct requirement * REQUIREMENT;

typedef struct capability * CAPABILITY;

typedef struct wire * WIRE;

struct bundle {
	BUNDLE_CONTEXT context;
	ACTIVATOR activator;
	BUNDLE_STATE state;
	void * handle;
	BUNDLE_ARCHIVE archive;
	// MODULE module;
	ARRAY_LIST modules;
	MANIFEST manifest;
	apr_pool_t *memoryPool;

	apr_thread_mutex_t *lock;
	int lockCount;
	apr_os_thread_t lockThread;

	struct framework * framework;
};

typedef struct bundle * BUNDLE;

struct serviceReference {
	BUNDLE bundle;
	struct serviceRegistration * registration;
};

typedef struct serviceReference * SERVICE_REFERENCE;

typedef enum serviceEventType
{
	REGISTERED = 0x00000001,
	MODIFIED = 0x00000002,
	UNREGISTERING = 0x00000004,
	MODIFIED_ENDMATCH = 0x00000008,
} SERVICE_EVENT_TYPE;

struct serviceEvent {
	SERVICE_REFERENCE reference;
	SERVICE_EVENT_TYPE type;
};

typedef struct serviceEvent * SERVICE_EVENT;

struct serviceRegistry {
    FRAMEWORK framework;
	HASH_MAP serviceRegistrations;
	HASH_MAP inUseMap;
	void (*serviceChanged)(FRAMEWORK, SERVICE_EVENT, PROPERTIES);
	long currentServiceId;


	pthread_mutex_t mutex;
};

typedef struct serviceRegistry * SERVICE_REGISTRY;

struct serviceRegistration {
	SERVICE_REGISTRY registry;
	char * className;
	SERVICE_REFERENCE reference;
	PROPERTIES properties;
	void * svcObj;
	long serviceId;

	pthread_mutex_t mutex;
	bool isUnregistering;

	bool isServiceFactory;
	void *serviceFactory;
};

typedef struct serviceRegistration * SERVICE_REGISTRATION;

struct serviceListener {
	void * handle;
	void (*serviceChanged)(void * listener, SERVICE_EVENT event);
};

typedef struct serviceListener * SERVICE_LISTENER;

struct serviceTrackerCustomizer {
	void * handle;
	void * (*addingService)(void * handle, SERVICE_REFERENCE reference);
	void (*addedService)(void * handle, SERVICE_REFERENCE reference, void * service);
	void (*modifiedService)(void * handle, SERVICE_REFERENCE reference, void * service);
	void (*removedService)(void * handle, SERVICE_REFERENCE reference, void * service);
};

typedef struct serviceTrackerCustomizer * SERVICE_TRACKER_CUSTOMIZER;

struct serviceTracker {
	BUNDLE_CONTEXT context;
	char * filter;
};

typedef struct serviceTracker * SERVICE_TRACKER;

#endif /* HEADERS_H_ */
