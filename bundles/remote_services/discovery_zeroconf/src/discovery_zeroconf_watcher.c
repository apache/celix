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
#include "discovery_zeroconf_watcher.h"
#include "discovery_zeroconf_constants.h"
#include "endpoint_listener.h"
#include "remote_constants.h"
#include "celix_bundle_context.h"
#include "celix_filter.h"
#include "celix_constants.h"
#include "celix_long_hash_map.h"
#include "celix_string_hash_map.h"
#include "celix_stdlib_cleanup.h"
#include "celix_threads.h"
#include "celix_unistd_cleanup.h"
#include "celix_utils.h"
#include "celix_errno.h"
#include <dns_sd.h>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <sys/param.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>

#define DZC_EP_JITTER_INTERVAL 10
#define DZC_MAX_RESOLVED_TIMEOUT 5
#define DZC_MAX_RESOLVED_CNT 10

struct discovery_zeroconf_watcher {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    long epListenerTrkId;
    char fwUuid[64];
    DNSServiceRef sharedRef;
    DNSServiceRef browseRef;
    int eventFd;
    celix_thread_t watchEPThread;
    celix_string_hash_map_t *watchedServices;//key:instanceName+interfaceId, val:watched_service_entry_t*
    celix_thread_mutex_t mutex;//projects below
    bool running;
    celix_string_hash_map_t *watchedEndpoints;//key:endpoint id, val:watched_endpoint_entry_t*
    celix_long_hash_map_t *epls;//key:service id, val:endpoint listener
};

typedef struct watched_endpoint_entry {
    endpoint_description_t *endpoint;
    struct timespec expiredTime;
}watched_endpoint_entry_t;

typedef struct watched_service_entry {
    celix_properties_t *txtRecord;
    const char *endpointId;
    int ifIndex;
    char instanceName[64];//The instanceName must be 1-63 bytes
    bool resolved;
    struct timespec resolvedStartTime;
    int resolvedCnt;
    DNSServiceRef resolveRef;
}watched_service_entry_t;

typedef struct watched_epl_entry {
    endpoint_listener_t *epl;
    celix_filter_t *filter;
}watched_epl_entry_t;

static void discoveryZeroconfWatcher_addEPL(void *handle, void *svc, const celix_properties_t *props);
static void discoveryZeroconfWatcher_removeEPL(void *handle, void *svc, const celix_properties_t *props);
static void OnServiceResolveCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *host, uint16_t port, uint16_t txtLen, const unsigned char *txtRecord, void *context);
static void OnServiceBrowseCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *instanceName, const char *regtype, const char *replyDomain, void *context);
static void *discoveryZeroconfWatcher_watchEPThread(void *data);

celix_status_t discoveryZeroconfWatcher_create(celix_bundle_context_t *ctx, celix_log_helper_t *logHelper, discovery_zeroconf_watcher_t **watcherOut) {
    celix_status_t status = CELIX_SUCCESS;
    celix_autofree discovery_zeroconf_watcher_t *watcher = (discovery_zeroconf_watcher_t *)calloc(1, sizeof(*watcher));
    if (watcher == NULL) {
        celix_logHelper_fatal(logHelper, "Watcher: Failed to alloc watcher.");
        return CELIX_ENOMEM;
    }
    watcher->logHelper = logHelper;
    watcher->ctx = ctx;
    watcher->sharedRef = NULL;
    watcher->browseRef = NULL;
    watcher->eventFd = eventfd(0, 0);
    if (watcher->eventFd < 0) {
        celix_logHelper_fatal(logHelper, "Watcher: Failed to open event fd, %d.", errno);
        return CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, errno);
    }
    celix_auto(celix_fd_t) eventFd = watcher->eventFd;

    const char *fwUuid = celix_bundleContext_getProperty(ctx, CELIX_FRAMEWORK_UUID, NULL);
    if (fwUuid == NULL || strlen(fwUuid) >= sizeof(watcher->fwUuid)) {
        celix_logHelper_fatal(logHelper, "Watcher: Failed to get framework uuid.");
        return CELIX_BUNDLE_EXCEPTION;
    }
    strcpy(watcher->fwUuid, fwUuid);

    status = celixThreadMutex_create(&watcher->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_fatal(logHelper, "Watcher: Failed to create mutex, %d.", status);
        return status;
    }
    celix_autoptr(celix_thread_mutex_t) mutex = &watcher->mutex;
    celix_string_hash_map_create_options_t epOpts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
    epOpts.storeKeysWeakly = true;
    celix_autoptr(celix_string_hash_map_t) watchedEndpoints =
        watcher->watchedEndpoints = celix_stringHashMap_createWithOptions(&epOpts);
    assert(watcher->watchedEndpoints);
    celix_string_hash_map_create_options_t svcOpts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
    celix_autoptr(celix_string_hash_map_t) watchedServices =
        watcher->watchedServices = celix_stringHashMap_createWithOptions(&svcOpts);
    assert(watcher->watchedServices != NULL);

    celix_autoptr(celix_long_hash_map_t) epls =
        watcher->epls = celix_longHashMap_create();
    assert(watcher->epls != NULL);

    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = OSGI_ENDPOINT_LISTENER_SERVICE;
    opts.filter.filter = "(!(DISCOVERY=true))";
    opts.callbackHandle = watcher;
    opts.addWithProperties = discoveryZeroconfWatcher_addEPL;
    opts.removeWithProperties = discoveryZeroconfWatcher_removeEPL;
    watcher->epListenerTrkId = celix_bundleContext_trackServicesWithOptionsAsync(ctx, &opts);
    if (watcher->epListenerTrkId < 0) {
        celix_logHelper_fatal(logHelper, "Watcher: Failed to register endpoint listener service.");
        return CELIX_BUNDLE_EXCEPTION;
    }

    watcher->running = true;
    status = celixThread_create(&watcher->watchEPThread,NULL, discoveryZeroconfWatcher_watchEPThread, watcher);
    if (status != CELIX_SUCCESS) {
        celix_bundleContext_stopTracker(ctx, watcher->epListenerTrkId);
        return status;
    }
    celixThread_setName(&watcher->watchEPThread, "DiscWatcher");

    celix_steal_ptr(epls);
    celix_steal_ptr(watchedServices);
    celix_steal_ptr(watchedEndpoints);
    celix_steal_ptr(mutex);
    celix_steal_fd(&eventFd);
    *watcherOut = celix_steal_ptr(watcher);
    return CELIX_SUCCESS;
}

void discoveryZeroconfWatcher_destroy(discovery_zeroconf_watcher_t *watcher) {
    celixThreadMutex_lock(&watcher->mutex);
    watcher->running= false;
    celixThreadMutex_unlock(&watcher->mutex);
    eventfd_t val = 1;
    eventfd_write(watcher->eventFd, val);
    celixThread_join(watcher->watchEPThread, NULL);
    celix_bundleContext_stopTrackerAsync(watcher->ctx, watcher->epListenerTrkId, NULL, NULL);
    celix_bundleContext_waitForAsyncStopTracker(watcher->ctx, watcher->epListenerTrkId);
    celix_longHashMap_destroy(watcher->epls);
    assert(celix_stringHashMap_size(watcher->watchedServices) == 0);
    celix_stringHashMap_destroy(watcher->watchedServices);
    assert(celix_stringHashMap_size(watcher->watchedEndpoints) == 0);
    celix_stringHashMap_destroy(watcher->watchedEndpoints);
    celixThreadMutex_destroy(&watcher->mutex);
    close(watcher->eventFd);
    free(watcher);
    return;
}

static void discoveryZeroconfWatcher_addEPL(void *handle, void *svc, const celix_properties_t *props) {
    assert(handle != NULL);
    assert(svc != NULL);
    assert(props != NULL);
    discovery_zeroconf_watcher_t *watcher = (discovery_zeroconf_watcher_t *)handle;
    endpoint_listener_t *epl = (endpoint_listener_t *)svc;
    long serviceId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    if (serviceId == -1) {
        return;
    }

    watched_epl_entry_t *eplEntry = (watched_epl_entry_t *)calloc(1, sizeof(*eplEntry));
    if (eplEntry == NULL) {
        return;
    }

    const char *scope = celix_properties_get(props, OSGI_ENDPOINT_LISTENER_SCOPE, NULL);//matching on empty filter is always true
    celix_filter_t *filter = scope == NULL ? NULL : celix_filter_create(scope);

    celixThreadMutex_lock(&watcher->mutex);
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedEndpoints, itr) {
        watched_endpoint_entry_t *epEntry  = (watched_endpoint_entry_t *)itr.value.ptrValue;
        bool matched = celix_filter_match(filter, epEntry->endpoint->properties);
        if (matched) {
            epl->endpointAdded(epl->handle, epEntry->endpoint, (char*)scope);
        }
    }

    eplEntry->epl = epl;
    eplEntry->filter = filter;
    celix_longHashMap_put(watcher->epls, serviceId, eplEntry);
    celixThreadMutex_unlock(&watcher->mutex);

    return;
}

static void discoveryZeroconfWatcher_removeEPL(void *handle, void *svc, const celix_properties_t *props) {
    assert(handle != NULL);
    assert(svc != NULL);
    assert(props != NULL);
    discovery_zeroconf_watcher_t *watcher = (discovery_zeroconf_watcher_t *)handle;
    endpoint_listener_t *epl = (endpoint_listener_t *)svc;
    long serviceId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    if (serviceId == -1) {
        return;
    }

    celixThreadMutex_lock(&watcher->mutex);
    watched_epl_entry_t *eplEntry = (watched_epl_entry_t *)celix_longHashMap_get(watcher->epls, serviceId);
    if (eplEntry != NULL) {
        CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedEndpoints, itr) {
            watched_endpoint_entry_t *epEntry = (watched_endpoint_entry_t *) itr.value.ptrValue;
            bool matched = celix_filter_match(eplEntry->filter, epEntry->endpoint->properties);
            if (matched) {
                epl->endpointRemoved(epl->handle, epEntry->endpoint, (char *) celix_filter_getFilterString(eplEntry->filter));
            }
        }
        celix_longHashMap_remove(watcher->epls, serviceId);
        celix_filter_destroy(eplEntry->filter);
        free(eplEntry);
    }
    celixThreadMutex_unlock(&watcher->mutex);

    return;
}

static void OnServiceResolveCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *host, uint16_t port, uint16_t txtLen, const unsigned char *txtRecord, void *context) {
    (void)sdRef;//unused
    (void)flags;//unused
    (void)interfaceIndex;//unused
    (void)host;//unused
    (void)port;//unused
    (void)fullname;//unused
    if (errorCode != kDNSServiceErr_NoError) {
        return;
    }
    watched_service_entry_t *svcEntry = (watched_service_entry_t *)context;
    assert(svcEntry != NULL);
    celix_properties_t *properties = svcEntry->txtRecord;
    int cnt = TXTRecordGetCount(txtLen, txtRecord);
    for (int i = 0; i < cnt; ++i) {
        char key[UINT8_MAX+1] = {0};
        char val[UINT8_MAX+1] = {0};
        const void *valPtr = NULL;
        uint8_t valLen = 0;
        DNSServiceErrorType err = TXTRecordGetItemAtIndex(txtLen, txtRecord, i, sizeof(key), key, &valLen, &valPtr);
        if (err != kDNSServiceErr_NoError) {
            return;
        }
        assert(valLen <= UINT8_MAX);
        memcpy(val, valPtr, valLen);
        celix_properties_set(properties, key, val);
    }
    svcEntry->endpointId = celix_properties_get(properties, OSGI_RSA_ENDPOINT_ID, NULL);

    long propSize = celix_properties_getAsLong(properties, DZC_SERVICE_PROPERTIES_SIZE_KEY, 0);
    if (propSize == celix_properties_size(properties)) {
        celix_properties_unset(properties, DZC_SERVICE_PROPERTIES_SIZE_KEY);//Service endpoint do not need it
        svcEntry->resolved = true;
    }
    return;
}

static void OnServiceBrowseCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *instanceName, const char *regtype, const char *replyDomain, void *context) {
    (void)sdRef;//unused
    (void)regtype;//unused
    (void)replyDomain;//unused
    discovery_zeroconf_watcher_t *watcher = (discovery_zeroconf_watcher_t *)context;
    assert(watcher != NULL);
    if (errorCode != kDNSServiceErr_NoError) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to browse service, %d.", errorCode);
        return;
    }

    watched_service_entry_t *svcEntry = NULL;
    if (instanceName == NULL || strlen(instanceName) >= sizeof(svcEntry->instanceName)) {
        celix_logHelper_error(watcher->logHelper, "Watcher: service name err.");
        return;
    }

    celix_logHelper_info(watcher->logHelper, "Watcher: %s  %s on interface %d.", (flags & kDNSServiceFlagsAdd) ? "Add" : "Remove", instanceName, interfaceIndex);

    char key[128]={0};
    (void)snprintf(key, sizeof(key), "%s%d", instanceName, (int)interfaceIndex);
    if (flags & kDNSServiceFlagsAdd) {
        if (celix_stringHashMap_hasKey(watcher->watchedServices, key)) {
            celix_logHelper_info(watcher->logHelper,"Watcher: %s already on interface %d.", instanceName, interfaceIndex);
            return;
        }
        svcEntry = (watched_service_entry_t *)calloc(1, sizeof(*svcEntry));
        if (svcEntry == NULL) {
            celix_logHelper_error(watcher->logHelper, "Watcher: Failed service entry.");
            return;
        }
        svcEntry->resolveRef = NULL;
        svcEntry->txtRecord = celix_properties_create();
        svcEntry->endpointId = NULL;
        svcEntry->resolved = false;
        strcpy(svcEntry->instanceName, instanceName);
        svcEntry->ifIndex = (int)interfaceIndex;
        svcEntry->resolvedStartTime.tv_sec = INT_MAX;
        svcEntry->resolvedCnt = 0;
        celix_stringHashMap_put(watcher->watchedServices, key, svcEntry);
    } else {
        svcEntry = (watched_service_entry_t *)celix_stringHashMap_get(watcher->watchedServices, key);
        if (svcEntry) {
            celix_stringHashMap_remove(watcher->watchedServices, key);
            if (svcEntry->resolveRef) {
                DNSServiceRefDeallocate(svcEntry->resolveRef);
            }
            celix_properties_destroy(svcEntry->txtRecord);
            free(svcEntry);
        }
    }
    return;
}

static void discoveryZeroconfWatcher_resolveServices(discovery_zeroconf_watcher_t *watcher) {
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedServices, iter) {
        watched_service_entry_t *svcEntry = (watched_service_entry_t *)iter.value.ptrValue;
        //If resolving is not completed for a long time，then close it, and try again later.
        if (svcEntry->resolved == false && svcEntry->resolveRef != NULL && celix_elapsedtime(CLOCK_MONOTONIC, svcEntry->resolvedStartTime) > DZC_MAX_RESOLVED_TIMEOUT) {
            DNSServiceRefDeallocate(svcEntry->resolveRef);
            svcEntry->resolveRef = NULL;
            svcEntry->resolvedStartTime.tv_sec = INT_MAX;
            svcEntry->resolvedStartTime.tv_nsec = 0;
            celix_logHelper_error(watcher->logHelper, "Watcher: resolve %s on %d timeout.", svcEntry->instanceName, svcEntry->ifIndex);
        }
        if (watcher->sharedRef && svcEntry->resolveRef == NULL && svcEntry->resolvedCnt < DZC_MAX_RESOLVED_CNT) {
            svcEntry->resolveRef = watcher->sharedRef;
            DNSServiceErrorType dnsErr = DNSServiceResolve(&svcEntry->resolveRef, kDNSServiceFlagsShareConnection , svcEntry->ifIndex, svcEntry->instanceName, DZC_SERVICE_PRIMARY_TYPE, "local", OnServiceResolveCallback, svcEntry);
            if (dnsErr != kDNSServiceErr_NoError) {
                svcEntry->resolveRef = NULL;
                celix_logHelper_error(watcher->logHelper, "Watcher: Failed to resolve %s on %d, %d.", svcEntry->instanceName, svcEntry->ifIndex, dnsErr);
            }
            svcEntry->resolvedStartTime = celix_gettime(CLOCK_MONOTONIC);
            svcEntry->resolvedCnt ++;
        }
    }
    return;
}

static void discoveryZeroConfWatcher_informEPLs(discovery_zeroconf_watcher_t *watcher, endpoint_description_t *endpoint, bool epAdded) {
    CELIX_LONG_HASH_MAP_ITERATE(watcher->epls, iter) {
        watched_epl_entry_t *eplEntry = (watched_epl_entry_t *)iter.value.ptrValue;
        endpoint_listener_t *epl = eplEntry->epl;
        bool matched = celix_filter_match(eplEntry->filter, endpoint->properties);
        if (matched) {
            if (epAdded) {
                epl->endpointAdded(epl->handle, endpoint, (char*)celix_filter_getFilterString(eplEntry->filter));
            } else {
                epl->endpointRemoved(epl->handle, endpoint, (char*)celix_filter_getFilterString(eplEntry->filter));
            }
        }
    }
    return;
}

static bool discoveryZeroconfWatcher_hasWatchedServiceForEndpoint(discovery_zeroconf_watcher_t *watcher, const char *endpointId) {
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedServices, iter) {
        watched_service_entry_t *svcEntry = (watched_service_entry_t *)iter.value.ptrValue;
        if (svcEntry->endpointId != NULL && strcmp(svcEntry->endpointId, endpointId) == 0) {
            return true;
        }
    }
    return false;
}

static void discoveryZeroconfWatcher_refreshEndpoints(discovery_zeroconf_watcher_t *watcher) {
    watched_endpoint_entry_t *epEntry = NULL;
    watched_service_entry_t *svcEntry = NULL;

    //remove self endpoints
    celix_string_hash_map_iterator_t svcIter = celix_stringHashMap_begin(watcher->watchedServices);
    while (!celix_stringHashMapIterator_isEnd(&svcIter)) {
        svcEntry = (watched_service_entry_t *)svcIter.value.ptrValue;
        if (svcEntry->resolved) {
            const char *epFwUuid = celix_properties_get(svcEntry->txtRecord, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, NULL);
            if (epFwUuid != NULL && strcmp(epFwUuid, watcher->fwUuid) == 0) {
                celix_logHelper_debug(watcher->logHelper, "Watcher: Ignore self endpoint for %s.", celix_properties_get(svcEntry->txtRecord, CELIX_FRAMEWORK_SERVICE_NAME, "unknown"));
                celix_stringHashMapIterator_remove(&svcIter);
                if (svcEntry->resolveRef) {
                    DNSServiceRefDeallocate(svcEntry->resolveRef);
                }
                celix_properties_destroy(svcEntry->txtRecord);
                free(svcEntry);
                continue;
            }
        }
        celix_stringHashMapIterator_next(&svcIter);
    }

    celixThreadMutex_lock(&watcher->mutex);

    //add new endpoint
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedServices, iter) {
        svcEntry = (watched_service_entry_t *)iter.value.ptrValue;
        if (svcEntry->endpointId != NULL && svcEntry->resolved) {
            epEntry = (watched_endpoint_entry_t *)celix_stringHashMap_get(watcher->watchedEndpoints, svcEntry->endpointId);
            if (epEntry == NULL) {
                celix_properties_t *properties = celix_properties_copy(svcEntry->txtRecord);
                endpoint_description_t *ep = NULL;
                celix_status_t status = endpointDescription_create(properties,&ep);
                if (status != CELIX_SUCCESS) {
                    celix_properties_destroy(properties);
                    // If properties invalid,endpointDescription_create will return error.
                    // Avoid endpointDescription_create again, set svcEntry->resolved false.
                    svcEntry->resolved = false;
                    continue;
                }
                celix_logHelper_debug(watcher->logHelper, "Watcher: Add endpoint for %s on %s.", ep->serviceName,ep->frameworkUUID);
                epEntry = (watched_endpoint_entry_t *)calloc(1, sizeof(*epEntry));
                if (epEntry == NULL) {
                    celix_logHelper_error(watcher->logHelper, "Watcher:Failed to alloc endpoint entry.");
                    //It will free ep and properties
                    endpointDescription_destroy(ep);
                    continue;
                }
                epEntry->endpoint = ep;
                discoveryZeroConfWatcher_informEPLs(watcher, ep, true);
                celix_stringHashMap_put(watcher->watchedEndpoints, epEntry->endpoint->id, epEntry);
            }
            epEntry->expiredTime.tv_sec = INT_MAX;
        }
    }

    //remove expired endpoint
    celix_string_hash_map_iterator_t epIter = celix_stringHashMap_begin(watcher->watchedEndpoints);
    while (!celix_stringHashMapIterator_isEnd(&epIter)) {
        epEntry = (watched_endpoint_entry_t *)epIter.value.ptrValue;
        const char *endpointId = epIter.key;
        if (!discoveryZeroconfWatcher_hasWatchedServiceForEndpoint(watcher, endpointId)) {
            if (epEntry->expiredTime.tv_sec == INT_MAX) {
                epEntry->expiredTime = celix_gettime(CLOCK_MONOTONIC);
            } else if (celix_elapsedtime(CLOCK_MONOTONIC, epEntry->expiredTime) >= DZC_EP_JITTER_INTERVAL) {
                celix_logHelper_debug(watcher->logHelper, "Watcher: Remove endpoint for %s on %s.", epEntry->endpoint->serviceName, epEntry->endpoint->frameworkUUID);
                celix_stringHashMapIterator_remove(&epIter);
                discoveryZeroConfWatcher_informEPLs(watcher, epEntry->endpoint, false);
                endpointDescription_destroy(epEntry->endpoint);
                free(epEntry);
                continue;
            }
        }
        celix_stringHashMapIterator_next(&epIter);
    }

    celixThreadMutex_unlock(&watcher->mutex);

    return;
}

static void discoveryZeroconfWatcher_clearEndpoints(discovery_zeroconf_watcher_t *watcher) {
    celixThreadMutex_lock(&watcher->mutex);
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedEndpoints, iter) {
        watched_endpoint_entry_t *epEntry = (watched_endpoint_entry_t *)iter.value.ptrValue;
        discoveryZeroConfWatcher_informEPLs(watcher, epEntry->endpoint, false);
        endpointDescription_destroy(epEntry->endpoint);
        free(epEntry);
    }
    celix_stringHashMap_clear(watcher->watchedEndpoints);
    celixThreadMutex_unlock(&watcher->mutex);
    return;
}

static void discoveryZeroconfWatcher_closeMDNSConnection(discovery_zeroconf_watcher_t *watcher) {
    if (watcher->sharedRef) {
        DNSServiceRefDeallocate(watcher->sharedRef);
        watcher->sharedRef = NULL;
        watcher->browseRef = NULL;//no need free entry->browseRef, 'DNSServiceRefDeallocate(watcher->sharedRef)' has do it.
    }
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedServices, iter) {
        watched_service_entry_t *svcEntry = (watched_service_entry_t *) iter.value.ptrValue;
        //no need free svcEntry->resolveRef, 'DNSServiceRefDeallocate(watcher->sharedRef)' has do it.
        celix_properties_destroy(svcEntry->txtRecord);
        free(svcEntry);
    }
    celix_stringHashMap_clear(watcher->watchedServices);
    return;
}

static void discoveryZeroconfWatcher_handleMDNSEvent(discovery_zeroconf_watcher_t *watcher) {
    DNSServiceErrorType dnsErr = DNSServiceProcessResult(watcher->sharedRef);
    if (dnsErr == kDNSServiceErr_ServiceNotRunning || dnsErr == kDNSServiceErr_DefunctConnection) {
        celix_logHelper_error(watcher->logHelper, "Watcher: mDNS connection may be broken, %d.", dnsErr);
        discoveryZeroconfWatcher_closeMDNSConnection(watcher);
    } else if (dnsErr != kDNSServiceErr_NoError) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to process mDNS result, %d.", dnsErr);
    }

    return;
}

static void *discoveryZeroconfWatcher_watchEPThread(void *data) {
    assert(data != NULL);
    discovery_zeroconf_watcher_t *watcher = (discovery_zeroconf_watcher_t *)data;
    DNSServiceErrorType dnsErr;
    fd_set readfds;
    eventfd_t val;
    int dsFd;
    int maxFd;
    struct timeval timeout;
    bool running = watcher->running;
    while (running) {
        if (watcher->sharedRef == NULL) {
            dnsErr = DNSServiceCreateConnection(&watcher->sharedRef);
            if (dnsErr != kDNSServiceErr_NoError) {
                celix_logHelper_error(watcher->logHelper, "Watcher: Failed to create connection for DNS service, %d.", dnsErr);
            }
        }

        if (watcher->sharedRef != NULL && watcher->browseRef == NULL) {
            watcher->browseRef = watcher->sharedRef;
            dnsErr = DNSServiceBrowse(&watcher->browseRef, kDNSServiceFlagsShareConnection, 0, DZC_SERVICE_PRIMARY_TYPE, "local", OnServiceBrowseCallback, watcher);
            if (dnsErr != kDNSServiceErr_NoError) {
                celix_logHelper_error(watcher->logHelper, "Watcher: Failed to browse DNS service, %d.", dnsErr);
                watcher->browseRef = NULL;
            }
        }

        FD_ZERO(&readfds);
        FD_SET(watcher->eventFd, &readfds);
        maxFd = watcher->eventFd;
        if (watcher->sharedRef) {
            dsFd = DNSServiceRefSockFD(watcher->sharedRef);
            assert(dsFd >= 0);
            FD_SET(dsFd, &readfds);
            maxFd = MAX(maxFd, dsFd);
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
        } else {
            dsFd = -1;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;
        }

        int result = select(maxFd+1, &readfds, NULL, NULL, &timeout);
        if (result > 0) {
            if (FD_ISSET(watcher->eventFd, &readfds)) {
                eventfd_read(watcher->eventFd, &val);
                //do nothing, the thread will be exited
            }

            if (dsFd >= 0 && FD_ISSET(dsFd, &readfds)) {
                discoveryZeroconfWatcher_handleMDNSEvent(watcher);
            }
        } else if (result == -1 && errno != EINTR) {
            celix_logHelper_error(watcher->logHelper, "Watcher: Error Selecting event, %d.", errno);
        }

        discoveryZeroconfWatcher_resolveServices(watcher);
        discoveryZeroconfWatcher_refreshEndpoints(watcher);

        celixThreadMutex_lock(&watcher->mutex);
        running = watcher->running;
        celixThreadMutex_unlock(&watcher->mutex);
    }
    discoveryZeroconfWatcher_closeMDNSConnection(watcher);
    discoveryZeroconfWatcher_clearEndpoints(watcher);

    return NULL;
}
