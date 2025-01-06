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
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <arpa/inet.h>

#define DZC_EP_JITTER_INTERVAL 5//The jitter interval for endpoint.Avoid updating endpoint description on all network interfaces when only one network interface is updated.The endpoint description will be removed when the service is removed for 5 seconds.
#define DZC_MAX_RESOLVED_CNT 10 //Max resolved count for each service when resolve service failed
#define DZC_MAX_RETRY_INTERVAL 5 //Max retry interval when resolve service failed

#define DZC_MAX_HOSTNAME_LEN 255 //The fully qualified domain name of the host, eg: "MyComputer.local". RFC 1034 specifies that this name is limited to 255 bytes.

#define DZC_MAX_SERVICE_INSTANCE_NAME_LEN 64 //The instanceName must be 1-63 bytes, + 1 for '\0'

struct discovery_zeroconf_watcher {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    char fwUuid[64];
    DNSServiceRef sharedRef;
    int eventFd;
    celix_thread_t watchEPThread;
    celix_string_hash_map_t *watchedServices;//key:instanceName+'/'+interfaceIndex, val:watched_service_entry_t*
    celix_string_hash_map_t *watchedHosts;//key:hostname+interfaceIndex, val:watched_host_entry_t*
    celix_thread_mutex_t mutex;//projects below
    bool running;
    celix_string_hash_map_t *watchedEndpoints;//key:endpoint id, val:watched_endpoint_entry_t*
    celix_long_hash_map_t *epls;//key:service id, val:endpoint listener
    celix_string_hash_map_t *serviceBrowsers;//key:service subtype(https://www.rfc-editor.org/rfc/rfc6763.html#section-7.1), val:service_browser_entry_t*
};

typedef struct watched_endpoint_entry {
    endpoint_description_t *endpoint;
    char *hostname;
    int ifIndex;
    const char *ipAddressesStr;
    struct timespec expiredTime;
}watched_endpoint_entry_t;

typedef struct watched_service_entry {
    celix_log_helper_t *logHelper;
    celix_properties_t *txtRecord;
    const char *endpointId;
    int ifIndex;
    char instanceName[DZC_MAX_SERVICE_INSTANCE_NAME_LEN];
    char *hostname;
    int port;
    bool resolved;
    int resolvedCnt;
    DNSServiceRef resolveRef;
    bool reResolve;
    bool markDeleted;
}watched_service_entry_t;

typedef struct watched_epl_entry {
    endpoint_listener_t *epl;
    celix_filter_t *filter;
}watched_epl_entry_t;

typedef struct service_browser_entry {
    DNSServiceRef browseRef;
    celix_log_helper_t *logHelper;
    celix_string_hash_map_t *watchedServices;//key:instanceName+'/'+interfaceIndex, val:interfaceIndex
    celix_long_hash_map_t* relatedListeners;//key:service id, val:null
    int resolvedCnt;
    bool markDeleted;
}service_browser_entry_t;

typedef struct watched_host_entry {
    DNSServiceRef sdRef;
    celix_log_helper_t *logHelper;
    char *hostname;
    int ifIndex;
    celix_string_hash_map_t  *ipAddresses;//key:ip address, val:true(ipv4)/false(ipv6)
    bool resolved;
    int resolvedCnt;
    bool markDeleted;
}watched_host_entry_t;

static void OnServiceResolveCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *host, uint16_t port, uint16_t txtLen, const unsigned char *txtRecord, void *context);
static void OnServiceBrowseCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *instanceName, const char *regtype, const char *replyDomain, void *context);
static void *discoveryZeroconfWatcher_watchEPThread(void *data);

celix_status_t discoveryZeroconfWatcher_create(celix_bundle_context_t *ctx, celix_log_helper_t *logHelper, discovery_zeroconf_watcher_t **watcherOut) {
    celix_status_t status = CELIX_SUCCESS;
    celix_autofree discovery_zeroconf_watcher_t *watcher = (discovery_zeroconf_watcher_t *)calloc(1, sizeof(*watcher));
    if (watcher == NULL) {
        celix_logHelper_error(logHelper, "Watcher: Failed to alloc watcher.");
        return CELIX_ENOMEM;
    }
    watcher->logHelper = logHelper;
    watcher->ctx = ctx;
    watcher->sharedRef = NULL;
    watcher->eventFd = eventfd(0, 0);
    if (watcher->eventFd < 0) {
        celix_logHelper_error(logHelper, "Watcher: Failed to open event fd, %d.", errno);
        return CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, errno);
    }
    celix_auto(celix_fd_t) eventFd = watcher->eventFd;

    const char *fwUuid = celix_bundleContext_getProperty(ctx, CELIX_FRAMEWORK_UUID, NULL);
    if (fwUuid == NULL || strlen(fwUuid) >= sizeof(watcher->fwUuid)) {
        celix_logHelper_error(logHelper, "Watcher: Failed to get framework uuid.");
        return CELIX_BUNDLE_EXCEPTION;
    }
    strcpy(watcher->fwUuid, fwUuid);

    status = celixThreadMutex_create(&watcher->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Watcher: Failed to create mutex, %d.", status);
        return status;
    }
    celix_autoptr(celix_thread_mutex_t) mutex = &watcher->mutex;
    celix_autoptr(celix_string_hash_map_t) serviceBrowsers = watcher->serviceBrowsers = celix_stringHashMap_create();
    if (serviceBrowsers == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "Watcher: Failed to create service browsers map.");
        return CELIX_ENOMEM;
    }
    celix_string_hash_map_create_options_t epOpts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
    epOpts.storeKeysWeakly = true;
    celix_autoptr(celix_string_hash_map_t) watchedEndpoints =
        watcher->watchedEndpoints = celix_stringHashMap_createWithOptions(&epOpts);
    if (watchedEndpoints == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "Watcher: Failed to create endpoints map.");
        return CELIX_ENOMEM;
    }
    celix_autoptr(celix_string_hash_map_t) watchedHosts = watcher->watchedHosts = celix_stringHashMap_create();
    if (watchedHosts == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "Watcher: Failed to create hosts map.");
        return CELIX_ENOMEM;
    }
    celix_autoptr(celix_string_hash_map_t) watchedServices =
        watcher->watchedServices = celix_stringHashMap_create();
    if (watchedServices == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "Watcher: Failed to create services map.");
        return CELIX_ENOMEM;
    }
    celix_autoptr(celix_long_hash_map_t) epls =
        watcher->epls = celix_longHashMap_create();
    if (epls == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "Watcher: Failed to create endpoint listener map.");
        return CELIX_ENOMEM;
    }

    watcher->running = true;
    status = celixThread_create(&watcher->watchEPThread,NULL, discoveryZeroconfWatcher_watchEPThread, watcher);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    celixThread_setName(&watcher->watchEPThread, "DiscWatcher");

    celix_steal_ptr(epls);
    celix_steal_ptr(watchedServices);
    celix_steal_ptr(watchedHosts);
    celix_steal_ptr(watchedEndpoints);
    celix_steal_ptr(serviceBrowsers);
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
    celix_longHashMap_destroy(watcher->epls);
    assert(celix_stringHashMap_size(watcher->watchedServices) == 0);
    celix_stringHashMap_destroy(watcher->watchedServices);
    assert(celix_stringHashMap_size(watcher->watchedHosts) == 0);
    celix_stringHashMap_destroy(watcher->watchedHosts);
    assert(celix_stringHashMap_size(watcher->watchedEndpoints) == 0);
    celix_stringHashMap_destroy(watcher->watchedEndpoints);
    CELIX_STRING_HASH_MAP_ITERATE(watcher->serviceBrowsers, iter) {
        service_browser_entry_t *browserEntry = (service_browser_entry_t *)iter.value.ptrValue;
        celix_stringHashMap_destroy(browserEntry->watchedServices);
        celix_longHashMap_destroy(browserEntry->relatedListeners);
        free(browserEntry);
    }
    celix_stringHashMap_destroy(watcher->serviceBrowsers);
    celixThreadMutex_destroy(&watcher->mutex);
    close(watcher->eventFd);
    free(watcher);
    return;
}

static bool discoveryZeroConfWatcher_addServiceBrowser(discovery_zeroconf_watcher_t *watcher, const char *serviceConfigType,
                                                       long listenerId) {
    const char *svcSubType = strrchr(serviceConfigType, '.');//We use the last word of the configuration type as mDNS service subtype
    if (svcSubType != NULL) {
        svcSubType += 1;//skip '.'
    } else {
        svcSubType = serviceConfigType;
    }
    size_t subTypeLen = strlen(svcSubType);
    if (subTypeLen > 63) {//the subtype identifier is allowed to be up to 63 bytes,https://www.rfc-editor.org/rfc/rfc6763.txt#section-7.2
        celix_logHelper_error(watcher->logHelper, "Watcher: Invalid service type for %s.", serviceConfigType);
        return false;
    }
    celix_auto(celix_mutex_lock_guard_t) lockGuard = celixMutexLockGuard_init(&watcher->mutex);
    celix_autofree service_browser_entry_t *browserEntry = (service_browser_entry_t *)celix_stringHashMap_get(watcher->serviceBrowsers, svcSubType);
    if (browserEntry != NULL) {
        if (celix_longHashMap_put(browserEntry->relatedListeners, listenerId, NULL) != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(watcher->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(watcher->logHelper, "Watcher: Failed to attach listener to existed service browser.");
        }
        celix_steal_ptr(browserEntry);
        return false;
    }
    browserEntry = (service_browser_entry_t *)calloc(1, sizeof(*browserEntry));
    if (browserEntry == NULL) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to alloc service browser entry.");
        return false;
    }
    celix_autoptr(celix_long_hash_map_t) relatedListeners = browserEntry->relatedListeners = celix_longHashMap_create();
    if (relatedListeners == NULL) {
        celix_logHelper_logTssErrors(watcher->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to create related listeners map.");
        return false;
    }
    if (celix_longHashMap_put(browserEntry->relatedListeners, listenerId, NULL) != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(watcher->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to attach listener to service browser.");
        return false;
    }
    celix_autoptr(celix_string_hash_map_t) watchedServices = browserEntry->watchedServices = celix_stringHashMap_create();
    if (watchedServices == NULL) {
        celix_logHelper_logTssErrors(watcher->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to create watched services map.");
        return false;
    }
    browserEntry->resolvedCnt = 0;
    browserEntry->browseRef = NULL;
    browserEntry->logHelper = watcher->logHelper;
    browserEntry->markDeleted = false;
    if (celix_stringHashMap_put(watcher->serviceBrowsers, svcSubType, browserEntry) != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(watcher->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to put service browser entry.");
        return false;
    }
    celix_steal_ptr(watchedServices);
    celix_steal_ptr(relatedListeners);
    celix_steal_ptr(browserEntry);
    return true;
}

static bool discoveryZeroConfWatcher_removeServiceBrowser(discovery_zeroconf_watcher_t* watcher, const char* serviceConfigType,
                                                          long listenerId) {
    const char *svcSubType = strrchr(serviceConfigType, '.');
    if (svcSubType != NULL) {
        svcSubType += 1;//skip '.'
    } else {
        svcSubType = serviceConfigType;
    }
    celix_auto(celix_mutex_lock_guard_t) lockGuard = celixMutexLockGuard_init(&watcher->mutex);
    service_browser_entry_t *browserEntry = (service_browser_entry_t *)celix_stringHashMap_get(watcher->serviceBrowsers, svcSubType);
    if ((browserEntry != NULL)) {
        celix_longHashMap_remove(browserEntry->relatedListeners, listenerId);
        return celix_longHashMap_size(browserEntry->relatedListeners) == 0;
    }
    return false;
}

int discoveryZeroconfWatcher_addEPL(void *handle, void *svc, const celix_properties_t *props) {
    assert(handle != NULL);
    assert(svc != NULL);
    assert(props != NULL);
    discovery_zeroconf_watcher_t *watcher = (discovery_zeroconf_watcher_t *)handle;
    endpoint_listener_t *epl = (endpoint_listener_t *)svc;
    long serviceId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    if (serviceId == -1) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to get endpoint listener service id.");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    watched_epl_entry_t *eplEntry = (watched_epl_entry_t *)calloc(1, sizeof(*eplEntry));
    if (eplEntry == NULL) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to alloc endpoint listener entry.");
        return CELIX_ENOMEM;
    }

    const char *scope = celix_properties_get(props, CELIX_RSA_ENDPOINT_LISTENER_SCOPE, NULL);//matching on empty filter is always true
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

    const char* listenedConfigType = celix_filter_findAttribute(filter, CELIX_RSA_SERVICE_IMPORTED_CONFIGS);
    if (listenedConfigType != NULL && discoveryZeroConfWatcher_addServiceBrowser(watcher, listenedConfigType, serviceId) == true) {
        eventfd_t val = 1;
        eventfd_write(watcher->eventFd, val);
    }

    return CELIX_SUCCESS;
}

int discoveryZeroconfWatcher_removeEPL(void *handle, void *svc, const celix_properties_t *props) {
    assert(handle != NULL);
    assert(svc != NULL);
    assert(props != NULL);
    discovery_zeroconf_watcher_t *watcher = (discovery_zeroconf_watcher_t *)handle;
    endpoint_listener_t *epl = (endpoint_listener_t *)svc;
    long serviceId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    if (serviceId == -1) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to get endpoint listener service id.");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_autoptr(celix_filter_t) filter = NULL;
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
        filter = celix_steal_ptr(eplEntry->filter);
        free(eplEntry);
    }
    celixThreadMutex_unlock(&watcher->mutex);

    const char* listenedConfigType = celix_filter_findAttribute(filter, CELIX_RSA_SERVICE_IMPORTED_CONFIGS);
    if (listenedConfigType != NULL && discoveryZeroConfWatcher_removeServiceBrowser(watcher, listenedConfigType, serviceId) == true) {
        eventfd_t val = 1;
        eventfd_write(watcher->eventFd, val);
    }

    return CELIX_SUCCESS;
}

int discoveryZeroConfWatcher_addRSA(void *handle, void *svc, const celix_properties_t *props) {
    assert(handle != NULL);
    assert(svc != NULL);
    assert(props != NULL);
    discovery_zeroconf_watcher_t *watcher = (discovery_zeroconf_watcher_t *)handle;
    long serviceId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    if (serviceId == -1) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to get remote service admin service id.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    const char *configsSupported = celix_properties_get(props, CELIX_RSA_REMOTE_CONFIGS_SUPPORTED, NULL);
    celix_autofree char *configsSupportedCopy = celix_utils_strdup(configsSupported);
    if (configsSupportedCopy == NULL) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to dup remote configs supported.");
        return CELIX_ENOMEM;
    }
    bool refreshBrowsers = false;
    char *token = strtok(configsSupportedCopy, ",");
    while (token != NULL) {
        token = celix_utils_trimInPlace(token);
        if (discoveryZeroConfWatcher_addServiceBrowser(watcher, token, serviceId)) {
            refreshBrowsers = true;
        }
        token = strtok(NULL, ",");
    }

    if (refreshBrowsers) {
        eventfd_t val = 1;
        eventfd_write(watcher->eventFd, val);
    }

    return CELIX_SUCCESS;
}

int discoveryZeroConfWatcher_removeRSA(void *handle, void *svc, const celix_properties_t *props) {
    assert(handle != NULL);
    assert(svc != NULL);
    assert(props != NULL);
    discovery_zeroconf_watcher_t *watcher = (discovery_zeroconf_watcher_t *)handle;
    long serviceId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    if (serviceId == -1) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to get remote service admin service id.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    const char *configsSupported = celix_properties_get(props, CELIX_RSA_REMOTE_CONFIGS_SUPPORTED, NULL);
    celix_autofree char *configsSupportedCopy = celix_utils_strdup(configsSupported);
    if (configsSupportedCopy == NULL) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to dup remote configs supported.");
        return CELIX_ENOMEM;
    }
    bool refreshBrowsers = false;
    char *token = strtok(configsSupportedCopy, ",");
    while (token != NULL) {
        token = celix_utils_trimInPlace(token);
        if (discoveryZeroConfWatcher_removeServiceBrowser(watcher, token, serviceId)) {
            refreshBrowsers = true;
        }
        token = strtok(NULL, ",");
    }

    if (refreshBrowsers) {
        eventfd_t val = 1;
        eventfd_write(watcher->eventFd, val);
    }

    return CELIX_SUCCESS;
}

static void OnServiceResolveCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *host, uint16_t port, uint16_t txtLen, const unsigned char *txtRecord, void *context) {
    (void)sdRef;//unused
    (void)flags;//unused
    (void)port;//unused
    (void)fullname;//unused
    watched_service_entry_t *svcEntry = (watched_service_entry_t *)context;
    assert(svcEntry != NULL);
    if (errorCode != kDNSServiceErr_NoError || strlen(host) > DZC_MAX_HOSTNAME_LEN) {
        celix_logHelper_error(svcEntry->logHelper, "Watcher: Failed to resolve service, or hostname invalid, %d.", errorCode);
        return;
    }
    if (interfaceIndex != svcEntry->ifIndex) {
        return;
    }
    celix_properties_t *properties = svcEntry->txtRecord;
    int cnt = TXTRecordGetCount(txtLen, txtRecord);
    for (int i = 0; i < cnt; ++i) {
        char key[UINT8_MAX+1] = {0};
        char val[UINT8_MAX+1] = {0};
        const void *valPtr = NULL;
        uint8_t valLen = 0;
        DNSServiceErrorType err = TXTRecordGetItemAtIndex(txtLen, txtRecord, i, sizeof(key), key, &valLen, &valPtr);
        if (err != kDNSServiceErr_NoError) {
            celix_logHelper_error(svcEntry->logHelper, "Watcher: Failed to get txt record item(%s), %d.", key, err);
            return;
        }
        assert(valLen <= UINT8_MAX);
        memcpy(val, valPtr, valLen);
        int status = celix_properties_set(properties, key, val);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(svcEntry->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(svcEntry->logHelper, "Watcher: Failed to set txt record item(%s), %d.", key, status);
            svcEntry->reResolve = (status == CELIX_ENOMEM) ? true : svcEntry->reResolve;
            return;
        }
    }
    svcEntry->endpointId = celix_properties_get(properties, CELIX_RSA_ENDPOINT_ID, NULL);

    long propSize = celix_properties_getAsLong(properties, DZC_SERVICE_PROPERTIES_SIZE_KEY, 0);
    const char *version = celix_properties_get(properties, DZC_TXT_RECORD_VERSION_KEY, "");
    if (propSize == celix_properties_size(properties) && strcmp(DZC_CURRENT_TXT_RECORD_VERSION, version) == 0) {
        svcEntry->port = ntohs(port);
        if (celix_properties_get(properties, CELIX_RSA_IP_ADDRESSES, NULL) != NULL) {//If no need fill in dynamic ip address, no need to resolve ip address
            free(svcEntry->hostname);//free old hostname
            svcEntry->hostname = celix_utils_strdup(host);
            if (svcEntry->hostname == NULL) {
                celix_logHelper_error(svcEntry->logHelper, "Watcher: Failed to dup hostname.");
                svcEntry->reResolve = true;
                return;
            }
        }

        celix_properties_unset(properties, DZC_SERVICE_PROPERTIES_SIZE_KEY);//Service endpoint do not need it
        celix_properties_unset(properties, DZC_TXT_RECORD_VERSION_KEY);//Service endpoint do not need it
        svcEntry->resolved = true;
        celix_logHelper_trace(svcEntry->logHelper, "Watcher: Resolved service %s on %u.", svcEntry->instanceName, interfaceIndex);
    }
    return;
}

static void OnServiceBrowseCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *instanceName, const char *regtype, const char *replyDomain, void *context) {
    (void)sdRef;//unused
    (void)regtype;//unused
    (void)replyDomain;//unused
    service_browser_entry_t *browser = (service_browser_entry_t *)context;
    assert(browser != NULL);
    if (errorCode != kDNSServiceErr_NoError) {
        celix_logHelper_error(browser->logHelper, "Watcher: Failed to browse service, %d.", errorCode);
        return;
    }

    if (instanceName == NULL || strlen(instanceName) >= DZC_MAX_SERVICE_INSTANCE_NAME_LEN) {
        celix_logHelper_error(browser->logHelper, "Watcher: service name err.");
        return;
    }

    celix_logHelper_info(browser->logHelper, "Watcher: %s  %s on interface %d.", (flags & kDNSServiceFlagsAdd) ? "Add" : "Remove", instanceName, interfaceIndex);

    char key[128]={0};
    (void)snprintf(key, sizeof(key), "%s/%d", instanceName, (int)interfaceIndex);
    if (flags & kDNSServiceFlagsAdd) {
        int status = celix_stringHashMap_putLong(browser->watchedServices, key, interfaceIndex);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(browser->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(browser->logHelper, "Watcher: Failed to cache service instance name, %d.", status);
        }
    } else {
        celix_stringHashMap_remove(browser->watchedServices, key);
    }
    return;
}

static void discoveryZeroconfWatcher_pickUpdatedServiceBrowsers(discovery_zeroconf_watcher_t *watcher, celix_string_hash_map_t *updatedServiceBrowsers, unsigned int *pNextWorkIntervalTime) {
    int status = CELIX_SUCCESS;
    unsigned int nextWorkIntervalTime = *pNextWorkIntervalTime;

    celixThreadMutex_lock(&watcher->mutex);
    celix_string_hash_map_iterator_t iter = celix_stringHashMap_begin(watcher->serviceBrowsers);
    while (!celix_stringHashMapIterator_isEnd(&iter)) {
        service_browser_entry_t *browserEntry = (service_browser_entry_t *)iter.value.ptrValue;
        bool browserToBeDeleted = celix_longHashMap_size(browserEntry->relatedListeners) == 0;
        if (watcher->sharedRef != NULL && browserEntry->browseRef == NULL && !browserToBeDeleted && browserEntry->resolvedCnt < DZC_MAX_RESOLVED_CNT) {
            status = celix_stringHashMap_put(updatedServiceBrowsers, iter.key, browserEntry);
            if (status != CELIX_SUCCESS) {
                nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry browse after 5 seconds
                celix_logHelper_logTssErrors(watcher->logHelper, CELIX_LOG_LEVEL_ERROR);
                celix_logHelper_error(watcher->logHelper, "Watcher: Failed to put browse entry, %d.", status);
            }
        } else if (browserToBeDeleted) {
            status = celix_stringHashMap_put(updatedServiceBrowsers, iter.key, browserEntry);
            if (status == CELIX_SUCCESS) {
                celix_stringHashMapIterator_remove(&iter);
                browserEntry->markDeleted = true;
                continue;
            } else {
                nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry browse after 5 seconds
                celix_logHelper_logTssErrors(watcher->logHelper, CELIX_LOG_LEVEL_ERROR);
                celix_logHelper_error(watcher->logHelper, "Watcher: Failed to put browse entry, %d.", status);
            }
        }
        celix_stringHashMapIterator_next(&iter);
    }
    celixThreadMutex_unlock(&watcher->mutex);

    //release to be deleted service browsers
    celix_string_hash_map_iterator_t iter2 = celix_stringHashMap_begin(updatedServiceBrowsers);
    while (!celix_stringHashMapIterator_isEnd(&iter2)) {
        service_browser_entry_t *browserEntry = (service_browser_entry_t *)iter2.value.ptrValue;
        if (browserEntry->markDeleted) {
            celix_logHelper_trace(watcher->logHelper, "Watcher: Stop to browse service type %s,%s.", DZC_SERVICE_PRIMARY_TYPE, iter2.key);
            celix_stringHashMapIterator_remove(&iter2);
            if (browserEntry->browseRef) {
                DNSServiceRefDeallocate(browserEntry->browseRef);
            }
            celix_stringHashMap_destroy(browserEntry->watchedServices);
            celix_longHashMap_destroy(browserEntry->relatedListeners);
            free(browserEntry);
        } else {
            celix_stringHashMapIterator_next(&iter2);
        }
    }

    *pNextWorkIntervalTime = nextWorkIntervalTime;
    return;
}

static void discoveryZeroconfWatcher_browseServices(discovery_zeroconf_watcher_t *watcher, celix_string_hash_map_t *updatedServiceBrowsers, unsigned int *pNextWorkIntervalTime) {
    unsigned int nextWorkIntervalTime = *pNextWorkIntervalTime;

    CELIX_STRING_HASH_MAP_ITERATE(updatedServiceBrowsers, iter) {
        service_browser_entry_t *browserEntry = (service_browser_entry_t *)iter.value.ptrValue;
        browserEntry->browseRef = watcher->sharedRef;
        char serviceType[128] = {0};//primary type(15bytes) + subtype(63bytes)
        (void)snprintf(serviceType, sizeof(serviceType), "%s,%s", DZC_SERVICE_PRIMARY_TYPE, iter.key);
        celix_logHelper_trace(watcher->logHelper, "Watcher: Start to browse service type %s.", serviceType);
        DNSServiceErrorType dnsErr = DNSServiceBrowse(&browserEntry->browseRef, kDNSServiceFlagsShareConnection, 0, serviceType, "local", OnServiceBrowseCallback, browserEntry);
        if (dnsErr != kDNSServiceErr_NoError) {
            celix_logHelper_error(watcher->logHelper, "Watcher: Failed to browse DNS service, %d.", dnsErr);
            browserEntry->browseRef = NULL;
            nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry browse after 5 seconds
        }
        browserEntry->resolvedCnt ++;
    }

    *pNextWorkIntervalTime = nextWorkIntervalTime;
    return;
}

static void discoveryZeroconfWatcher_resolveServices(discovery_zeroconf_watcher_t *watcher, unsigned int *pNextWorkIntervalTime) {
    unsigned int nextWorkIntervalTime = *pNextWorkIntervalTime;

    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedServices, iter) {
        watched_service_entry_t *svcEntry = (watched_service_entry_t *)iter.value.ptrValue;
        if (watcher->sharedRef && svcEntry->resolveRef == NULL && svcEntry->resolvedCnt < DZC_MAX_RESOLVED_CNT) {
            celix_logHelper_trace(watcher->logHelper, "Watcher: Start to resolve service %s on %d.", svcEntry->instanceName, svcEntry->ifIndex);
            svcEntry->resolveRef = watcher->sharedRef;
            DNSServiceErrorType dnsErr = DNSServiceResolve(&svcEntry->resolveRef, kDNSServiceFlagsShareConnection , svcEntry->ifIndex, svcEntry->instanceName, DZC_SERVICE_PRIMARY_TYPE, "local", OnServiceResolveCallback, svcEntry);
            if (dnsErr != kDNSServiceErr_NoError) {
                svcEntry->resolveRef = NULL;
                celix_logHelper_error(watcher->logHelper, "Watcher: Failed to resolve %s on %d, %d.", svcEntry->instanceName, svcEntry->ifIndex, dnsErr);
                nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry resolve after 5 seconds
            }
            svcEntry->resolvedCnt ++;
        }

        if (svcEntry->reResolve) {//release resolveRef and retry resolve service after 5 seconds
            svcEntry->reResolve = false;
            if (svcEntry->resolveRef) {
                DNSServiceRefDeallocate(svcEntry->resolveRef);
                svcEntry->resolveRef = NULL;
            }
            svcEntry->resolvedCnt = 0;
            svcEntry->resolved = false;
            nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry resolve after 5 seconds
        }
    }
    *pNextWorkIntervalTime = nextWorkIntervalTime;
    return;
}

static void discoveryZeroconfWatcher_refreshWatchedServices(discovery_zeroconf_watcher_t *watcher, unsigned int *pNextWorkIntervalTime) {
    unsigned int nextWorkIntervalTime = *pNextWorkIntervalTime;

    //mark deleted status for all watched services
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedServices, iter) {
        watched_service_entry_t *svcEntry = (watched_service_entry_t *)iter.value.ptrValue;
        svcEntry->markDeleted = true;
    }

    celixThreadMutex_lock(&watcher->mutex);
    CELIX_STRING_HASH_MAP_ITERATE(watcher->serviceBrowsers, iter) {
        service_browser_entry_t *browserEntry = (service_browser_entry_t *)iter.value.ptrValue;
        CELIX_STRING_HASH_MAP_ITERATE(browserEntry->watchedServices, iter2) {
            const char *key = iter2.key;
            int interfaceIndex = (int)iter2.value.longValue;
            celix_autofree watched_service_entry_t *svcEntry = (watched_service_entry_t *)celix_stringHashMap_get(watcher->watchedServices, key);
            if (svcEntry != NULL) {
                svcEntry->markDeleted = false;
                celix_steal_ptr(svcEntry);
                continue;
            }
            char *instanceNameEnd = strrchr(key, '/');
            if (instanceNameEnd == NULL || instanceNameEnd-key >= DZC_MAX_SERVICE_INSTANCE_NAME_LEN) {
                celix_logHelper_error(watcher->logHelper, "Watcher: Invalid service instance key, %s.", key);
                continue;
            }
            svcEntry = (watched_service_entry_t *)calloc(1, sizeof(*svcEntry));
            if (svcEntry == NULL) {
                celix_logHelper_error(watcher->logHelper, "Watcher: Failed to alloc service entry.");
                nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry browse after 5 seconds
                continue;
            }
            svcEntry->resolveRef = NULL;
            svcEntry->txtRecord = celix_properties_create();
            if (svcEntry->txtRecord == NULL) {
                celix_logHelper_logTssErrors(watcher->logHelper, CELIX_LOG_LEVEL_ERROR);
                celix_logHelper_error(watcher->logHelper, "Watcher: Failed to create txt record for service entry.");
                nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry browse after 5 seconds
                continue;
            }
            svcEntry->endpointId = NULL;
            svcEntry->resolved = false;
            strncpy(svcEntry->instanceName, key, instanceNameEnd-key);
            svcEntry->instanceName[instanceNameEnd-key] = '\0';
            svcEntry->hostname = NULL;
            svcEntry->ifIndex = interfaceIndex;
            svcEntry->resolvedCnt = 0;
            svcEntry->reResolve = false;
            svcEntry->markDeleted = false;
            svcEntry->logHelper = watcher->logHelper;
            int status = celix_stringHashMap_put(watcher->watchedServices, key, svcEntry);
            if (status != CELIX_SUCCESS) {
                celix_properties_destroy(svcEntry->txtRecord);
                celix_logHelper_logTssErrors(watcher->logHelper, CELIX_LOG_LEVEL_ERROR);
                celix_logHelper_error(watcher->logHelper, "Watcher: Failed to put service entry, %d.", status);
                nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry browse after 5 seconds
                continue;
            }
            celix_steal_ptr(svcEntry);
        }
    }
    celixThreadMutex_unlock(&watcher->mutex);

    //remove to be deleted services
    celix_string_hash_map_iterator_t iter = celix_stringHashMap_begin(watcher->watchedServices);
    while (!celix_stringHashMapIterator_isEnd(&iter)) {
        watched_service_entry_t *svcEntry = (watched_service_entry_t *)iter.value.ptrValue;
        if (svcEntry->markDeleted) {
            celix_logHelper_trace(watcher->logHelper, "Watcher: Stop to resolve service %s on %d.", svcEntry->instanceName, svcEntry->ifIndex);
            celix_stringHashMapIterator_remove(&iter);
            if (svcEntry->resolveRef) {
                DNSServiceRefDeallocate(svcEntry->resolveRef);
            }
            celix_properties_destroy(svcEntry->txtRecord);
            free(svcEntry->hostname);
            free(svcEntry);
        } else {
            celix_stringHashMapIterator_next(&iter);
        }
    }

    discoveryZeroconfWatcher_resolveServices(watcher, &nextWorkIntervalTime);

    *pNextWorkIntervalTime = nextWorkIntervalTime;
    return;
}

static void onGetAddrInfoCb (DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t ifIndex, DNSServiceErrorType errorCode,
                             const char *hostname, const struct sockaddr *address, uint32_t ttl, void *context) {
    (void)sdRef;//unused
    (void)ttl;//unused
    (void)hostname;//unused
    int status = CELIX_SUCCESS;
    watched_host_entry_t *hostEntry = (watched_host_entry_t *)context;
    assert(hostEntry != NULL);
    if (errorCode != kDNSServiceErr_NoError || address == NULL || (address->sa_family != AF_INET && address->sa_family != AF_INET6)) {
        celix_logHelper_error(hostEntry->logHelper, "Watcher: Failed to resolve host %s on %d, %d.", hostEntry->hostname, hostEntry->ifIndex, errorCode);
        return;
    }
    if (ifIndex != hostEntry->ifIndex) {
        return;
    }

    char ip[INET6_ADDRSTRLEN] = {0};
    if (address->sa_family == AF_INET) {
        inet_ntop(AF_INET, &((struct sockaddr_in *)address)->sin_addr, ip, sizeof(ip));
    } else if (address->sa_family == AF_INET6) {
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)address)->sin6_addr, ip, sizeof(ip));
    }

    if (flags & kDNSServiceFlagsAdd) {
        status = celix_stringHashMap_putBool(hostEntry->ipAddresses, ip, address->sa_family == AF_INET);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(hostEntry->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(hostEntry->logHelper, "Watcher: Failed to add ip address(%s). %d.", ip, status);
        }
    } else {
        celix_stringHashMap_remove(hostEntry->ipAddresses, ip);
    }

    celix_logHelper_trace(hostEntry->logHelper, "Watcher: %s ip %s for host %s on %d.", (flags & kDNSServiceFlagsAdd) ? "Add" : "Remove", ip, hostEntry->hostname, hostEntry->ifIndex);

    hostEntry->resolved = !(flags & kDNSServiceFlagsMoreComing);

    return;
}

static watched_host_entry_t *discoveryZeroconfWatcher_getHostEntry(discovery_zeroconf_watcher_t *watcher, const char *hostname, int ifIndex) {
    char key[256 + 10] = {0};//max hostname length is 255, ifIndex is int
    (void)snprintf(key, sizeof(key), "%s%d", hostname, ifIndex);
    return (watched_host_entry_t *)celix_stringHashMap_get(watcher->watchedHosts, key);
}

static void discoveryZeroconfWatcher_updateWatchedHosts(discovery_zeroconf_watcher_t *watcher, unsigned int *pNextWorkIntervalTime) {
    unsigned int nextWorkIntervalTime = *pNextWorkIntervalTime;
    //mark deleted hosts
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedHosts, iter) {
        watched_host_entry_t *hostEntry = (watched_host_entry_t *)iter.value.ptrValue;
        hostEntry->markDeleted = true;
    }

    //add or mark hosts
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedServices, iter) {
        watched_service_entry_t *svcEntry = (watched_service_entry_t *) iter.value.ptrValue;
        if (svcEntry->hostname != NULL && svcEntry->ifIndex != kDNSServiceInterfaceIndexLocalOnly) {
            celix_autofree watched_host_entry_t *hostEntry = discoveryZeroconfWatcher_getHostEntry(watcher, svcEntry->hostname, svcEntry->ifIndex);
            if (hostEntry != NULL) {
                hostEntry->markDeleted = false;
                celix_steal_ptr(hostEntry);
            } else {
                hostEntry = (watched_host_entry_t *)calloc(1, sizeof(*hostEntry));
                if (hostEntry == NULL) {
                    nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry after 5 seconds
                    celix_logHelper_error(watcher->logHelper, "Watcher: Failed to alloc host entry for %s.", svcEntry->instanceName);
                    continue;
                }
                celix_autoptr(celix_string_hash_map_t) ipAddresses = hostEntry->ipAddresses = celix_stringHashMap_create();
                if (ipAddresses == NULL) {
                    nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry after 5 seconds
                    celix_logHelper_error(watcher->logHelper, "Watcher: Failed to alloc ip address list for %s.", svcEntry->instanceName);
                    continue;
                }
                celix_autofree char *hostname = hostEntry->hostname = celix_utils_strdup(svcEntry->hostname);
                if (hostname == NULL) {
                    nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry after 5 seconds
                    celix_logHelper_error(watcher->logHelper, "Watcher: Failed to dup hostname for %s.", svcEntry->instanceName);
                    continue;
                }
                hostEntry->sdRef = NULL;
                hostEntry->logHelper = watcher->logHelper;
                hostEntry->resolved = false;
                hostEntry->ifIndex = svcEntry->ifIndex;
                hostEntry->resolvedCnt = 0;
                hostEntry->markDeleted = false;
                char key[256 + 10] = {0};//max hostname length is 255, ifIndex is int
                (void)snprintf(key, sizeof(key), "%s%d", svcEntry->hostname, svcEntry->ifIndex);
                if (celix_stringHashMap_put(watcher->watchedHosts, key, hostEntry) != CELIX_SUCCESS) {
                    nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry after 5 seconds
                    celix_logHelper_logTssErrors(watcher->logHelper, CELIX_LOG_LEVEL_ERROR);
                    celix_logHelper_error(watcher->logHelper, "Watcher: Failed to add host entry for %s.", svcEntry->instanceName);
                    continue;
                }
                celix_steal_ptr(hostname);
                celix_steal_ptr(ipAddresses);
                celix_steal_ptr(hostEntry);
            }
        }
    }

    //delete hosts
    celix_string_hash_map_iterator_t iter = celix_stringHashMap_begin(watcher->watchedHosts);
    while (!celix_stringHashMapIterator_isEnd(&iter)) {
        watched_host_entry_t *hostEntry = (watched_host_entry_t *)iter.value.ptrValue;
        if (hostEntry->markDeleted) {
            celix_logHelper_trace(watcher->logHelper, "Watcher: Stop to resolve host %s on %d.", hostEntry->hostname, hostEntry->ifIndex);
            celix_stringHashMapIterator_remove(&iter);
            if (hostEntry->sdRef) {
                DNSServiceRefDeallocate(hostEntry->sdRef);
            }
            celix_stringHashMap_destroy(hostEntry->ipAddresses);
            free(hostEntry->hostname);
            free(hostEntry);
        } else {
            celix_stringHashMapIterator_next(&iter);
        }
    }

    *pNextWorkIntervalTime = nextWorkIntervalTime;
    return;
}

static void discoveryZeroconfWatcher_refreshHostsInfo(discovery_zeroconf_watcher_t *watcher, unsigned int *pNextWorkIntervalTime) {
    unsigned int nextWorkIntervalTime = *pNextWorkIntervalTime;

    discoveryZeroconfWatcher_updateWatchedHosts(watcher, &nextWorkIntervalTime);

    //resolve hosts
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedHosts, iter1) {
        watched_host_entry_t *hostEntry = (watched_host_entry_t *)iter1.value.ptrValue;
        if (watcher->sharedRef && hostEntry->sdRef == NULL && hostEntry->resolvedCnt < DZC_MAX_RESOLVED_CNT) {
            celix_logHelper_trace(watcher->logHelper, "Watcher: Start to resolve host %s on %d.", hostEntry->hostname, hostEntry->ifIndex);
            hostEntry->sdRef = watcher->sharedRef;
            DNSServiceErrorType dnsErr = DNSServiceGetAddrInfo(&hostEntry->sdRef, kDNSServiceFlagsShareConnection, hostEntry->ifIndex, kDNSServiceProtocol_IPv4 | kDNSServiceProtocol_IPv6, hostEntry->hostname, onGetAddrInfoCb, hostEntry);
            if (dnsErr != kDNSServiceErr_NoError) {
                hostEntry->sdRef = NULL;
                celix_logHelper_error(watcher->logHelper, "Watcher: Failed to get address info for %s on %d, %d.", hostEntry->hostname, hostEntry->ifIndex, dnsErr);
                nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry resolve after 5 seconds
            }
            hostEntry->resolvedCnt ++;
        }
    }

    *pNextWorkIntervalTime = nextWorkIntervalTime;
    return;
}

static bool discoveryZeroConfWatcher_isHostResolved(discovery_zeroconf_watcher_t *watcher, const char *hostname, int ifIndex) {
    if (hostname == NULL || ifIndex == kDNSServiceInterfaceIndexLocalOnly) {//if no need to resolve host info, then return true
        return true;
    }
    watched_host_entry_t *hostEntry = discoveryZeroconfWatcher_getHostEntry(watcher, hostname, ifIndex);
    if (hostEntry == NULL) {
        return false;
    }
    if (celix_stringHashMap_size(hostEntry->ipAddresses) == 0) {
        return false;
    }

    return hostEntry->resolved;
}

static int discoveryZeroConfWatcher_getHostIpAddresses(discovery_zeroconf_watcher_t *watcher, const char *hostname, int ifIndex, char **ipAddressesStrOut) {
    celix_autofree char *ipAddressesStr = NULL;
    if (ifIndex == kDNSServiceInterfaceIndexLocalOnly) {
        *ipAddressesStrOut = celix_utils_strdup("127.0.0.1,::1");
        return (*ipAddressesStrOut != NULL) ? CELIX_SUCCESS : CELIX_ENOMEM;
    }
    watched_host_entry_t *hostEntry = discoveryZeroconfWatcher_getHostEntry(watcher, hostname, ifIndex);
    if (hostEntry != NULL) {
        CELIX_STRING_HASH_MAP_ITERATE(hostEntry->ipAddresses, iter) {
            const char *ip = iter.key;
            char *tmp= NULL;
            if (ipAddressesStr == NULL) {
                tmp = celix_utils_strdup(ip);
            } else {
                asprintf(&tmp, "%s,%s", ipAddressesStr, ip);
            }
            if (tmp == NULL) {
                return CELIX_ENOMEM;
            }
            free(ipAddressesStr);
            ipAddressesStr = tmp;
        }
    }
    if (ipAddressesStr == NULL) {
        return CELIX_ILLEGAL_STATE;
    }
    *ipAddressesStrOut = celix_steal_ptr(ipAddressesStr);
    return CELIX_SUCCESS;
}

static int discoveryZeroConfWatcher_createEndpointEntryForService(discovery_zeroconf_watcher_t *watcher, watched_service_entry_t *svcEntry, watched_endpoint_entry_t **epOut) {
    celix_autoptr(celix_properties_t) properties = celix_properties_copy(svcEntry->txtRecord);
    if (properties == NULL) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to copy endpoint properties.");
        return CELIX_ENOMEM;
    }
    celix_autoptr(endpoint_description_t) ep = NULL;
    int status = endpointDescription_create(properties, &ep);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to create endpoint description. %d.", status);
        return status;
    }
    celix_steal_ptr(properties);//properties now owned by ep

    celix_autofree watched_endpoint_entry_t *epEntry = (watched_endpoint_entry_t *)calloc(1, sizeof(*epEntry));
    if (epEntry == NULL) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to alloc endpoint entry.");
        return CELIX_ENOMEM;
    }
    epEntry->endpoint = ep;
    epEntry->ifIndex = svcEntry->ifIndex;
    epEntry->expiredTime.tv_sec = INT_MAX;
    epEntry->ipAddressesStr = NULL;
    epEntry->hostname = NULL;

    if (svcEntry->hostname == NULL) {//if no need to resolve host info, then return CELIX_SUCCESS
        celix_steal_ptr(ep);
        *epOut = celix_steal_ptr(epEntry);
        return CELIX_SUCCESS;
    }

    celix_autofree char *hostname = epEntry->hostname = celix_utils_strdup(svcEntry->hostname);
    if (hostname == NULL) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to dup hostname for endpoint %s.", svcEntry->instanceName);
        return CELIX_ENOMEM;
    }

    celix_autofree char *ipAddressesStr = NULL;
    status = discoveryZeroConfWatcher_getHostIpAddresses(watcher, hostname, svcEntry->ifIndex, &ipAddressesStr);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to get ip addresses for endpoint %s.", svcEntry->instanceName);
        return status;
    }

    status = celix_properties_setLong(ep->properties, CELIX_RSA_PORT, svcEntry->port);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to set imported config port.");
        return status;
    }
    status = celix_properties_set(ep->properties, CELIX_RSA_IP_ADDRESSES, ipAddressesStr);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to set imported config ip address list.");
        return status;
    }
    epEntry->ipAddressesStr = celix_properties_get(ep->properties, CELIX_RSA_IP_ADDRESSES, "");

    celix_steal_ptr(hostname);
    celix_steal_ptr(ep);
    *epOut = celix_steal_ptr(epEntry);

    return CELIX_SUCCESS;
}

static void endpointEntry_destroy(watched_endpoint_entry_t *entry) {
    endpointDescription_destroy(entry->endpoint);
    free(entry->hostname);
    free(entry);
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

static void discoveryZeroconfWatcher_filterSameFrameWorkServices(discovery_zeroconf_watcher_t *watcher) {
    celix_string_hash_map_iterator_t iter = celix_stringHashMap_begin(watcher->watchedServices);
    while (!celix_stringHashMapIterator_isEnd(&iter)) {
        watched_service_entry_t *svcEntry = (watched_service_entry_t *)iter.value.ptrValue;
        if (svcEntry->resolved) {
            const char *epFwUuid = celix_properties_get(svcEntry->txtRecord, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, NULL);
            if (epFwUuid != NULL && strcmp(epFwUuid, watcher->fwUuid) == 0) {
                celix_logHelper_debug(watcher->logHelper, "Watcher: Ignore self endpoint for %s.", celix_properties_get(svcEntry->txtRecord, CELIX_FRAMEWORK_SERVICE_NAME, "unknown"));
                celix_stringHashMapIterator_remove(&iter);

                //remove service instance name from service browser
                char instanceNameKey[128]={0};
                (void)snprintf(instanceNameKey, sizeof(instanceNameKey), "%s/%d", svcEntry->instanceName, svcEntry->ifIndex);
                celixThreadMutex_lock(&watcher->mutex);
                CELIX_STRING_HASH_MAP_ITERATE(watcher->serviceBrowsers, iter2) {
                    service_browser_entry_t *browserEntry = (service_browser_entry_t *)iter2.value.ptrValue;
                    celix_stringHashMap_remove(browserEntry->watchedServices, instanceNameKey);
                }
                celixThreadMutex_unlock(&watcher->mutex);

                //release service info
                if (svcEntry->resolveRef) {
                    DNSServiceRefDeallocate(svcEntry->resolveRef);
                }
                celix_properties_destroy(svcEntry->txtRecord);
                free(svcEntry->hostname);
                free(svcEntry);
                continue;
            }
        }
        celix_stringHashMapIterator_next(&iter);
    }
    return;
}

static bool discoveryZeroconfWatcher_checkEndpointIpAddressesChanged(discovery_zeroconf_watcher_t *watcher, watched_endpoint_entry_t *endpointEntry) {
    if (endpointEntry->hostname == NULL || endpointEntry->ifIndex == kDNSServiceInterfaceIndexLocalOnly) {
        return false;
    }
    watched_host_entry_t *hostEntry = discoveryZeroconfWatcher_getHostEntry(watcher, endpointEntry->hostname, endpointEntry->ifIndex);
    if (hostEntry == NULL) {
        return false;
    }
    if (!hostEntry->resolved) {
        return false;
    }
    if (endpointEntry->ipAddressesStr != NULL) {
        const char *p = endpointEntry->ipAddressesStr;
        const char *end = p + strlen(p);
        char ip[INET6_ADDRSTRLEN] = {0};
        int i = 0;
        int ipCnt = 0;
        while (p <= end && i < INET6_ADDRSTRLEN){
            ip[i] = (*p == ',') ? '\0' : *p;
            if (ip[i++] == '\0') {
                if (!celix_stringHashMap_hasKey(hostEntry->ipAddresses, ip)) {
                    return true;
                }
                i = 0;
                ipCnt ++;
            }
            p++;
        }
        if (ipCnt != celix_stringHashMap_size(hostEntry->ipAddresses)) {
            return true;
        }
    }
    return false;
}

static bool endpointEntry_matchServiceEntry(watched_endpoint_entry_t *epEntry, watched_service_entry_t *svcEntry) {
    if (epEntry->ifIndex != svcEntry->ifIndex) {
        return false;
    }
    if (epEntry->hostname == NULL && svcEntry->hostname == NULL) {
        return true;
    }
    if (epEntry->hostname != NULL && svcEntry->hostname != NULL && strcmp(epEntry->hostname, svcEntry->hostname) == 0) {
        return true;
    }
    return false;
}

static void discoveryZeroconfWatcher_refreshEndpoints(discovery_zeroconf_watcher_t *watcher, unsigned int *pNextWorkIntervalTime) {
    watched_endpoint_entry_t *epEntry = NULL;
    watched_service_entry_t *svcEntry = NULL;
    unsigned int nextWorkIntervalTime = *pNextWorkIntervalTime;

    celix_auto(celix_mutex_lock_guard_t) lockGuard = celixMutexLockGuard_init(&watcher->mutex);

    struct timespec now = celix_gettime(CLOCK_MONOTONIC);
    //remove the endpoint which ip address list changed and expired endpoint, and mark expired time of the endpoint.
    celix_string_hash_map_iterator_t epIter = celix_stringHashMap_begin(watcher->watchedEndpoints);
    while (!celix_stringHashMapIterator_isEnd(&epIter)) {
        epEntry = (watched_endpoint_entry_t *)epIter.value.ptrValue;
        if (discoveryZeroconfWatcher_checkEndpointIpAddressesChanged(watcher, epEntry)
            || celix_compareTime(&now, &epEntry->expiredTime) >= 0) {
            celix_logHelper_debug(watcher->logHelper, "Watcher: Remove endpoint for %s on %d.", epEntry->endpoint->serviceName, epEntry->ifIndex);
            celix_stringHashMapIterator_remove(&epIter);
            discoveryZeroConfWatcher_informEPLs(watcher, epEntry->endpoint, false);
            endpointEntry_destroy(epEntry);
        } else {
            if (epEntry->expiredTime.tv_sec == INT_MAX) {
                epEntry->expiredTime = celix_gettime(CLOCK_MONOTONIC);
                epEntry->expiredTime.tv_sec += DZC_EP_JITTER_INTERVAL;
            }
            celix_stringHashMapIterator_next(&epIter);
        }
    }

    //add new endpoint
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedServices, iter) {
        svcEntry = (watched_service_entry_t *)iter.value.ptrValue;
        if (svcEntry->endpointId != NULL && svcEntry->resolved) {
            epEntry = (watched_endpoint_entry_t *)celix_stringHashMap_get(watcher->watchedEndpoints, svcEntry->endpointId);
            if (epEntry == NULL && discoveryZeroConfWatcher_isHostResolved(watcher, svcEntry->hostname, svcEntry->ifIndex)) {
                celix_status_t status = discoveryZeroConfWatcher_createEndpointEntryForService(watcher, svcEntry, &epEntry);
                if (status != CELIX_SUCCESS) {
                    // If properties invalid,endpointDescription_create will return error.
                    // Avoid create endpointDescription again, set svcEntry->resolved false.
                    if (status != CELIX_ENOMEM) {
                        svcEntry->resolved = false;
                    } else {
                        nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry after 5 seconds
                    }
                    celix_logHelper_error(watcher->logHelper, "Watcher: Failed to create endpoint for %s. %d.", svcEntry->instanceName, status);
                    continue;
                }
                celix_logHelper_debug(watcher->logHelper, "Watcher: Add endpoint for %s on %d.", epEntry->endpoint->serviceName, epEntry->ifIndex);
                if (celix_stringHashMap_put(watcher->watchedEndpoints, epEntry->endpoint->id, epEntry) == CELIX_SUCCESS) {
                    discoveryZeroConfWatcher_informEPLs(watcher, epEntry->endpoint, true);
                } else {
                    celix_logHelper_logTssErrors(watcher->logHelper, CELIX_LOG_LEVEL_ERROR);
                    celix_logHelper_error(watcher->logHelper, "Watcher: Failed to add endpoint for %s.", epEntry->endpoint->serviceName);
                    endpointEntry_destroy(epEntry);
                    nextWorkIntervalTime = MIN(nextWorkIntervalTime, DZC_MAX_RETRY_INTERVAL);//retry after 5 seconds
                }
            } else if (epEntry != NULL && endpointEntry_matchServiceEntry(epEntry, svcEntry)) {
                epEntry->expiredTime.tv_sec = INT_MAX;
                epEntry->expiredTime.tv_nsec = 0;
            }
        }
    }

    //calculate next work time
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedEndpoints, iter) {
        epEntry = (watched_endpoint_entry_t *)iter.value.ptrValue;
        if (epEntry->expiredTime.tv_sec != INT_MAX) {
            int timeOut = celix_difftime(&now, &epEntry->expiredTime) + 1;
            assert(timeOut >= 0);//We have removed expired endpoint before.
            nextWorkIntervalTime = MIN(nextWorkIntervalTime, timeOut);
        }
    }

    *pNextWorkIntervalTime = nextWorkIntervalTime;
    return;
}

static void discoveryZeroconfWatcher_clearEndpoints(discovery_zeroconf_watcher_t *watcher) {
    celixThreadMutex_lock(&watcher->mutex);
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedEndpoints, iter) {
        watched_endpoint_entry_t *epEntry = (watched_endpoint_entry_t *)iter.value.ptrValue;
        discoveryZeroConfWatcher_informEPLs(watcher, epEntry->endpoint, false);
        endpointEntry_destroy(epEntry);
    }
    celix_stringHashMap_clear(watcher->watchedEndpoints);
    celixThreadMutex_unlock(&watcher->mutex);
    return;
}

static void discoveryZeroconfWatcher_closeMDNSConnection(discovery_zeroconf_watcher_t *watcher) {
    if (watcher->sharedRef) {
        DNSServiceRefDeallocate(watcher->sharedRef);
        watcher->sharedRef = NULL;
    }

    celixThreadMutex_lock(&watcher->mutex);
    CELIX_STRING_HASH_MAP_ITERATE(watcher->serviceBrowsers, iter) {
        service_browser_entry_t *browseEntry = (service_browser_entry_t *)iter.value.ptrValue;
        browseEntry->browseRef = NULL;//no need free V->browseRef, 'DNSServiceRefDeallocate(watcher->sharedRef)' has done it.
        browseEntry->resolvedCnt = 0;
        celix_stringHashMap_clear(browseEntry->watchedServices);
    }
    celixThreadMutex_unlock(&watcher->mutex);

    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedServices, iter) {
        watched_service_entry_t *svcEntry = (watched_service_entry_t *) iter.value.ptrValue;
        //no need free svcEntry->resolveRef, 'DNSServiceRefDeallocate(watcher->sharedRef)' has done it.
        celix_properties_destroy(svcEntry->txtRecord);
        free(svcEntry->hostname);
        free(svcEntry);
    }
    celix_stringHashMap_clear(watcher->watchedServices);
    CELIX_STRING_HASH_MAP_ITERATE(watcher->watchedHosts, iter) {
        watched_host_entry_t *hostEntry = (watched_host_entry_t *) iter.value.ptrValue;
        //no need free hostEntry->sdRef, 'DNSServiceRefDeallocate(watcher->sharedRef)' has done it.
        celix_stringHashMap_destroy(hostEntry->ipAddresses);
        free(hostEntry->hostname);
        free(hostEntry);
    }
    celix_stringHashMap_clear(watcher->watchedHosts);
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
    struct timeval timeoutVal;
    struct timeval *timeout = NULL;
    unsigned int timeoutInS = UINT_MAX;
    bool running = watcher->running;

    celix_autoptr(celix_string_hash_map_t) updatedBrowsers = celix_stringHashMap_create();
    if (updatedBrowsers == NULL) {
        celix_logHelper_logTssErrors(watcher->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(watcher->logHelper, "Watcher: Failed to create updated service browsers cache.");
        return NULL;
    }

    while (running) {
        if (watcher->sharedRef == NULL) {
            dnsErr = DNSServiceCreateConnection(&watcher->sharedRef);
            if (dnsErr != kDNSServiceErr_NoError) {
                celix_logHelper_error(watcher->logHelper, "Watcher: Failed to create connection for DNS service, %d.", dnsErr);
                timeoutInS = MIN(DZC_MAX_RETRY_INTERVAL, timeoutInS);//retry after 5 seconds
            } else {
                timeoutInS = 0;//need to browse service immediately when connection created
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
        } else {
            dsFd = -1;
        }

        if (timeoutInS == UINT_MAX) {
            timeout = NULL;//wait until eventfd or dsFd ready
        } else {
            timeoutVal.tv_sec = timeoutInS;
            timeoutVal.tv_usec = 0;
            timeout = &timeoutVal;
        }

        int result = select(maxFd+1, &readfds, NULL, NULL, timeout);
        if (result > 0) {
            if (FD_ISSET(watcher->eventFd, &readfds)) {
                eventfd_read(watcher->eventFd, &val);
                //the thread will be exited or browse service
            }

            if (dsFd >= 0 && FD_ISSET(dsFd, &readfds)) {
                discoveryZeroconfWatcher_handleMDNSEvent(watcher);
            }
        } else if (result == -1 && errno != EINTR) {
            celix_logHelper_error(watcher->logHelper, "Watcher: Error Selecting event, %d.", errno);
            sleep(1);//avoid busy loop
        }

        timeoutInS = UINT_MAX;
        discoveryZeroconfWatcher_pickUpdatedServiceBrowsers(watcher, updatedBrowsers, &timeoutInS);
        if (celix_stringHashMap_size(updatedBrowsers) > 0) {
            discoveryZeroconfWatcher_browseServices(watcher, updatedBrowsers, &timeoutInS);
            celix_stringHashMap_clear(updatedBrowsers);
        }
        discoveryZeroconfWatcher_refreshWatchedServices(watcher, &timeoutInS);
        discoveryZeroconfWatcher_filterSameFrameWorkServices(watcher);
        discoveryZeroconfWatcher_refreshHostsInfo(watcher, &timeoutInS);
        discoveryZeroconfWatcher_refreshEndpoints(watcher, &timeoutInS);

        celixThreadMutex_lock(&watcher->mutex);
        running = watcher->running;
        celixThreadMutex_unlock(&watcher->mutex);
    }
    discoveryZeroconfWatcher_closeMDNSConnection(watcher);
    discoveryZeroconfWatcher_clearEndpoints(watcher);

    return NULL;
}
