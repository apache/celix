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
#include "discovery_zeroconf_announcer.h"
#include "discovery_zeroconf_constants.h"
#include "endpoint_listener.h"
#include "remote_constants.h"
#include "celix_utils.h"
#include "celix_properties.h"
#include "celix_constants.h"
#include "celix_threads.h"
#include "celix_string_hash_map.h"
#include "celix_array_list.h"
#include "celix_log_helper.h"
#include "celix_errno.h"
#include "celix_build_assert.h"
#include "celix_stdlib_cleanup.h"
#include "celix_unistd_cleanup.h"
#include <dns_sd.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>


//According to rfc6763, Using TXT records larger than 1300 bytes is NOT RECOMMENDED
#define DZC_MAX_TXT_RECORD_SIZE 1300

//It is enough to store three subtypes.
#define DZC_MAX_SERVICE_TYPE_LEN 256

struct discovery_zeroconf_announcer {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    pid_t pid;
    DNSServiceRef sharedRef;
    int eventFd;
    celix_thread_t refreshEPThread;
    celix_thread_mutex_t mutex;//projects below
    bool running;
    celix_string_hash_map_t *endpoints;//key:endpoint id, val:announce_endpoint_entry_t*
    celix_array_list_t *revokedEndpoints;
    celix_string_hash_map_t *conflictCntMap;//key:service name, val:conflictCnt. Use to resolve conflict service name in same host.Because the mDNSResponder does not automatically resolve service name conflicts in the same host.
};

typedef struct announce_endpoint_entry {
    celix_properties_t *properties;
    DNSServiceRef registerRef;
    int ifIndex;
    int port;
    const char *serviceName;
    char *serviceType;
    bool announced;
    uint32_t conflictCnt;
}announce_endpoint_entry_t;

static void endpointEntry_destroy(announce_endpoint_entry_t *entry);
static void discoveryZeroconfAnnouncer_eventNotify(discovery_zeroconf_announcer_t *announcer);
static void *discoveryZeroconfAnnouncer_refreshEndpointThread(void *data);


celix_status_t discoveryZeroconfAnnouncer_create(celix_bundle_context_t *ctx, celix_log_helper_t *logHelper, discovery_zeroconf_announcer_t **announcerOut) {
    celix_status_t status = CELIX_SUCCESS;
    celix_autofree discovery_zeroconf_announcer_t *announcer = (discovery_zeroconf_announcer_t *)calloc(1, sizeof(*announcer));
    if (announcer == NULL) {
        celix_logHelper_error(logHelper, "Announcer: Failed to alloc announcer.");
        return CELIX_ENOMEM;
    }
    announcer->ctx = ctx;
    announcer->logHelper = logHelper;
    announcer->pid = getpid();
    announcer->sharedRef = NULL;

    announcer->eventFd = eventfd(0, 0);
    if (announcer->eventFd < 0) {
        celix_logHelper_error(logHelper, "Announcer: Failed to open event fd, %d.", errno);
        return CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, errno);
    }
    celix_auto(celix_fd_t) eventFd = announcer->eventFd;

    status = celixThreadMutex_create(&announcer->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Announcer: Failed to create mutex, %d.", status);
        return status;
    }
    celix_autoptr(celix_thread_mutex_t) mutex = &announcer->mutex;

    celix_autoptr(celix_string_hash_map_t) endpoints = announcer->endpoints = celix_stringHashMap_create();
    if (endpoints == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "Announcer: Failed to create endpoints map.");
        return CELIX_ENOMEM;
    }
    celix_autoptr(celix_array_list_t) revokedEndpoints = announcer->revokedEndpoints = celix_arrayList_create();
    if (revokedEndpoints == NULL) {
        celix_logHelper_error(logHelper, "Announcer: Failed to create revoked endpoints list.");
        return CELIX_ENOMEM;
    }

    celix_autoptr(celix_string_hash_map_t) conflictCntMap = announcer->conflictCntMap = celix_stringHashMap_create();
    if (conflictCntMap == NULL) {
        celix_logHelper_error(logHelper, "Announcer: Failed to create conflict count map.");
        return CELIX_ENOMEM;
    }

    announcer->running = true;
    status = celixThread_create(&announcer->refreshEPThread, NULL, discoveryZeroconfAnnouncer_refreshEndpointThread, announcer);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Announcer: Failed to create refreshing endpoint thread, %d.", status);
        return status;
    }
    celixThread_setName(&announcer->refreshEPThread, "DiscAnnouncer");

    celix_steal_ptr(conflictCntMap);
    celix_steal_ptr(revokedEndpoints);
    celix_steal_ptr(endpoints);
    celix_steal_ptr(mutex);
    celix_steal_fd(&eventFd);
    *announcerOut = celix_steal_ptr(announcer);
    return CELIX_SUCCESS;
}

void discoveryZeroconfAnnouncer_destroy(discovery_zeroconf_announcer_t *announcer) {
    celixThreadMutex_lock(&announcer->mutex);
    announcer->running= false;
    celixThreadMutex_unlock(&announcer->mutex);
    discoveryZeroconfAnnouncer_eventNotify(announcer);
    celixThread_join(announcer->refreshEPThread, NULL);

    celix_stringHashMap_destroy(announcer->conflictCntMap);
    announce_endpoint_entry_t *entry = NULL;
    int size = celix_arrayList_size(announcer->revokedEndpoints);
    for (int i = 0; i < size; ++i) {
        entry = (announce_endpoint_entry_t *)celix_arrayList_get(announcer->revokedEndpoints, i);
        endpointEntry_destroy(entry);
    }
    celix_arrayList_destroy(announcer->revokedEndpoints);

    CELIX_STRING_HASH_MAP_ITERATE(announcer->endpoints,iter) {
        entry = (announce_endpoint_entry_t *) iter.value.ptrValue;
        endpointEntry_destroy(entry);
    }
    celix_stringHashMap_destroy(announcer->endpoints);

    celixThreadMutex_destroy(&announcer->mutex);
    close(announcer->eventFd);
    free(announcer);
    return;
}

static void discoveryZeroconfAnnouncer_eventNotify(discovery_zeroconf_announcer_t *announcer) {
    eventfd_t val = 1;
    eventfd_write(announcer->eventFd, val);
    return;
}

static bool isLoopBackNetInterface(const char *ifName) {
    if (strlen(ifName) >= IF_NAMESIZE) {
        return false;
    }
    bool loopBack = false;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd >= 0) {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strcpy(ifr.ifr_name, ifName);
        if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0) {
            loopBack = !!(ifr.ifr_ifru.ifru_flags & IFF_LOOPBACK);
        }
        close(fd);
    }
    return loopBack;
}

static int discoveryZeroconfAnnouncer_setServiceSubTypeTo(discovery_zeroconf_announcer_t *announcer, const char *importedConfigType, celix_string_hash_map_t *svcSubTypes) {
    //We use the last word of config type as mDNS service subtype(https://www.rfc-editor.org/rfc/rfc6763.html#section-7.1). so we can browse the service by the last word of config type.
    const char *svcSubType = strrchr(importedConfigType, '.');
    if (svcSubType != NULL) {
        svcSubType += 1;//skip '.'
    } else {
        svcSubType = importedConfigType;
    }
    size_t subTypeLen = strlen(svcSubType);
    if (subTypeLen ==0 || subTypeLen > 63) {//the subtype identifier is allowed to be up to 63 bytes, see https://www.rfc-editor.org/rfc/rfc6763.html#section-7.2
        celix_logHelper_error(announcer->logHelper, "Announcer: Invalid service sub type for %s.", importedConfigType);
        return CELIX_ILLEGAL_ARGUMENT;
    }
    int status = celix_stringHashMap_put(svcSubTypes, svcSubType, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(announcer->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(announcer->logHelper, "Announcer: Failed to put service sub type for %s. %d", importedConfigType, status);
        return status;
    }
    return CELIX_SUCCESS;
}

static char* discoveryZeroconfAnnouncer_createServiceTypeAccordingToConfigTypes(discovery_zeroconf_announcer_t* announcer, const char* configTypes) {
    celix_string_hash_map_create_options_t opts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
    opts.storeKeysWeakly = true;
    celix_autoptr(celix_string_hash_map_t) svcSubTypes = celix_stringHashMap_createWithOptions(&opts);
    if (svcSubTypes == NULL) {
        celix_logHelper_logTssErrors(announcer->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(announcer->logHelper, "Announcer: Failed to create svc sub types map.");
        return NULL;
    }
    celix_autofree char *configTypesCopy = celix_utils_strdup(configTypes);
    if (configTypesCopy == NULL) {
        celix_logHelper_error(announcer->logHelper, "Announcer: Failed to dup config types.");
        return NULL;
    }
    char *savePtr = NULL;
    char *token = strtok_r(configTypesCopy, ",", &savePtr);
    while (token != NULL) {
        token = celix_utils_trimInPlace(token);
        int status = discoveryZeroconfAnnouncer_setServiceSubTypeTo(announcer, token, svcSubTypes);
        if (status != CELIX_SUCCESS) {
            return NULL;
        }
        token = strtok_r(NULL, ",", &savePtr);
    }

    char serviceType[DZC_MAX_SERVICE_TYPE_LEN] = {0};
    CELIX_BUILD_ASSERT(sizeof(serviceType) >= sizeof(DZC_SERVICE_PRIMARY_TYPE));
    strcpy(serviceType, DZC_SERVICE_PRIMARY_TYPE);
    size_t offset = strlen(serviceType);
    CELIX_STRING_HASH_MAP_ITERATE(svcSubTypes, iter) {
        offset += snprintf(serviceType + offset, sizeof(serviceType) - offset, ",%s", iter.key);
        if (offset >= sizeof(serviceType)) {
            celix_logHelper_error(announcer->logHelper, "Announcer: Please reduce the length of imported configs for %s.", serviceType);
            return NULL;
        }
    }
    return celix_utils_strdup(serviceType);
}

static int discoveryZeroconfAnnouncer_createEndpointEntry(discovery_zeroconf_announcer_t *announcer, endpoint_description_t *endpoint, announce_endpoint_entry_t **entryOut) {
    celix_autoptr(celix_properties_t) properties = celix_properties_copy(endpoint->properties);
    if (properties == NULL) {
        celix_logHelper_logTssErrors(announcer->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(announcer->logHelper, "Announcer: Failed to copy endpoint properties for %s.", endpoint->serviceName);
        return CELIX_ENOMEM;
    }
    const char* serviceName = celix_properties_get(properties, CELIX_FRAMEWORK_SERVICE_NAME, NULL);
    if (serviceName == NULL) {
        celix_logHelper_error(announcer->logHelper,"Announcer: Invalid service.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    const char *importedConfigs = celix_properties_get(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, NULL);
    if (importedConfigs == NULL) {
        celix_logHelper_error(announcer->logHelper, "Announcer: No imported configs for %s.", serviceName);
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_autofree announce_endpoint_entry_t *entry = (announce_endpoint_entry_t *)calloc(1, sizeof(*entry));
    if (entry == NULL) {
        celix_logHelper_error(announcer->logHelper, "Announcer:  Failed to alloc endpoint entry for %s.", serviceName);
        return CELIX_ENOMEM;
    }
    entry->registerRef = NULL;
    entry->announced = false;
    entry->conflictCnt = 0;
    const char *ifName = celix_properties_get(properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, NULL);
    if (ifName != NULL) {
        if (strcmp(ifName, "all") == 0) {
            entry->ifIndex = kDNSServiceInterfaceIndexAny;
        } else if (isLoopBackNetInterface(ifName)) {
            // If it is a loopback interface,we will announce the service on the local only interface.
            // Because the mDNSResponder will skip the loopback interface,if it found a normal interface.
            entry->ifIndex = kDNSServiceInterfaceIndexLocalOnly;
        } else {
            entry->ifIndex = (int)if_nametoindex(ifName);
            if (entry->ifIndex == 0) {
                celix_logHelper_error(announcer->logHelper, "Announcer: Invalid network interface name %s.", ifName);
                return CELIX_ILLEGAL_ARGUMENT;
            }
        }
    } else {
        entry->ifIndex = DZC_SERVICE_ANNOUNCED_IF_INDEX_DEFAULT;
    }
    entry->port = celix_properties_getAsLong(properties, CELIX_RSA_PORT, DZC_PORT_DEFAULT);

    entry->serviceType = discoveryZeroconfAnnouncer_createServiceTypeAccordingToConfigTypes(announcer, importedConfigs);
    if (entry->serviceType == NULL) {
        celix_logHelper_error(announcer->logHelper, "Announcer: Failed to create service type for %s.", serviceName);
        return CELIX_ENOMEM;
    }
    entry->serviceName = serviceName;
    celix_properties_unset(properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE);//ifname should not set to mDNS TXT record, because service consumer will not use it.
    celix_properties_unset(properties, CELIX_RSA_PORT);//port should not set to mDNS TXT record, because it will be set to SRV record. see https://www.rfc-editor.org/rfc/rfc6763.html#section-6.3
    entry->properties = celix_steal_ptr(properties);
    *entryOut = celix_steal_ptr(entry);
    return CELIX_SUCCESS;
}

static void endpointEntry_destroy(announce_endpoint_entry_t *entry) {
    celix_properties_destroy(entry->properties);
    free(entry->serviceType);
    free(entry);
    return;
}
CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(announce_endpoint_entry_t, endpointEntry_destroy)

static int discoveryZeroconfAnnouncer_generateServiceNameConflictCnt(discovery_zeroconf_announcer_t *announcer, const char *serviceName, uint32_t *conflictCntOut) {
    uint32_t conflictCnt = (uint32_t)celix_stringHashMap_getLong(announcer->conflictCntMap, serviceName, 0);
    bool conflicted;
    do {
        conflicted = false;
        conflictCnt++;
        CELIX_STRING_HASH_MAP_ITERATE(announcer->endpoints, iter) {
            announce_endpoint_entry_t *entry = (announce_endpoint_entry_t *)iter.value.ptrValue;
            if (strcmp(entry->serviceName, serviceName) == 0 && entry->conflictCnt == conflictCnt) {
                conflicted = true;
                break;
            }
        }
    }while(conflicted);

    int status = celix_stringHashMap_putLong(announcer->conflictCntMap, serviceName, conflictCnt);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(announcer->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(announcer->logHelper, "Announcer: Failed to update conflict count for %s.", serviceName);
        return status;
    }
    *conflictCntOut = conflictCnt;
    return CELIX_SUCCESS;
}

celix_status_t discoveryZeroconfAnnouncer_endpointAdded(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
    (void)matchedFilter;//unused
    int status = CELIX_SUCCESS;
    discovery_zeroconf_announcer_t *announcer = (discovery_zeroconf_announcer_t *)handle;
    assert(announcer != NULL);
    if (endpointDescription_isInvalid(endpoint)) {
        celix_logHelper_error(announcer->logHelper, "Announcer: Endpoint is invalid.");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_logHelper_info(announcer->logHelper, "Announcer: Add endpoint for %s(%s).", endpoint->serviceName, endpoint->id);

    celix_autoptr(announce_endpoint_entry_t) entry = NULL;
    status = discoveryZeroconfAnnouncer_createEndpointEntry(announcer, endpoint, &entry);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    celix_auto(celix_mutex_lock_guard_t) lockGuard = celixMutexLockGuard_init(&announcer->mutex);
    status = discoveryZeroconfAnnouncer_generateServiceNameConflictCnt(announcer, entry->serviceName, &entry->conflictCnt);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    status = celix_stringHashMap_put(announcer->endpoints, endpoint->id, entry);
    if (status == CELIX_SUCCESS) {
        celix_steal_ptr(entry);
    } else {
        celix_logHelper_logTssErrors(announcer->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(announcer->logHelper, "Announcer: Failed to put endpoint entry for %s.", endpoint->id);
        return status;
    }

    discoveryZeroconfAnnouncer_eventNotify(announcer);

    return CELIX_SUCCESS;
}

celix_status_t discoveryZeroconfAnnouncer_endpointRemoved(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
    (void)matchedFilter;//unused
    discovery_zeroconf_announcer_t *announcer = (discovery_zeroconf_announcer_t *)handle;
    assert(announcer != NULL);
    if (endpointDescription_isInvalid(endpoint)) {
        celix_logHelper_error(announcer->logHelper, "Announcer: Endpoint is invalid.");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_logHelper_info(announcer->logHelper, "Announcer: Remove endpoint for %s(%s).", endpoint->serviceName, endpoint->id);

    celixThreadMutex_lock(&announcer->mutex);
    announce_endpoint_entry_t *entry = (announce_endpoint_entry_t *)celix_stringHashMap_get(announcer->endpoints, endpoint->id);
    if (entry) {
        (void)celix_stringHashMap_remove(announcer->endpoints, endpoint->id);
        celix_arrayList_add(announcer->revokedEndpoints, entry);
    }
    celix_stringHashMap_putLong(announcer->conflictCntMap, endpoint->serviceName, 0);//reset conflict count
    celixThreadMutex_unlock(&announcer->mutex);
    discoveryZeroconfAnnouncer_eventNotify(announcer);
    return CELIX_SUCCESS;
}

static void OnDNSServiceRegisterCallback(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *instanceName, const char *serviceType, const char *domain, void *data) {
    (void)sdRef;//unused
    discovery_zeroconf_announcer_t *announcer = (discovery_zeroconf_announcer_t *)data;
    assert(announcer != NULL);
    if (errorCode == kDNSServiceErr_NoError) {
        celix_logHelper_info(announcer->logHelper, "Announcer: Got a reply for service %s.%s%s: %s.", instanceName, serviceType, domain, (flags & kDNSServiceFlagsAdd) ? "Registered" : "Removed");
    } else {
        celix_logHelper_error(announcer->logHelper, "Announcer: Failed to register service, %d.", errorCode);
    }
    return;
}


static void discoveryZeroconfAnnouncer_revokeEndpoints(discovery_zeroconf_announcer_t *announcer, celix_array_list_t *endpoints) {
    (void)announcer;//unused
    announce_endpoint_entry_t *entry = NULL;
    int size = celix_arrayList_size(endpoints);
    for (int i = 0; i < size; ++i) {
        entry = celix_arrayList_get(endpoints, i);
        if (entry->registerRef != NULL) {
            celix_logHelper_info(announcer->logHelper, "Announcer: Deregister service %s on interface %d.", entry->serviceName, entry->ifIndex);
            DNSServiceRefDeallocate(entry->registerRef);
        }
        endpointEntry_destroy(entry);
    }
    return;
}

static bool discoveryZeroconfAnnouncer_copyPropertiesToTxtRecord(discovery_zeroconf_announcer_t *announcer, celix_properties_iterator_t *propIter, TXTRecordRef *txtRecord, uint16_t maxTxtLen, bool splitTxtRecord) {
    const char *key;
    const char *val;
    while (!celix_propertiesIterator_isEnd(propIter)) {
        key = propIter->key;
        val = propIter->entry.value;
        if (key) {
            DNSServiceErrorType err = TXTRecordSetValue(txtRecord, key, strlen(val), val);
            if (err != kDNSServiceErr_NoError) {
                celix_logHelper_error(announcer->logHelper, "Announcer: Failed to set txt value, %d.", err);
                return false;
            }
            if (splitTxtRecord && TXTRecordGetLength(txtRecord) >= maxTxtLen - UINT8_MAX) {
                celix_propertiesIterator_next(propIter);
                break;
            }
        }
        celix_propertiesIterator_next(propIter);
    }
    return true;
}

static void discoveryZeroconfAnnouncer_announceEndpoints(discovery_zeroconf_announcer_t *announcer, celix_array_list_t *endpoints) {
    announce_endpoint_entry_t *entry = NULL;
    int size = celix_arrayList_size(endpoints);
    for (int i = 0; i < size; ++i) {
        entry = celix_arrayList_get(endpoints, i);
        entry->announced = true;//Set the flag first, avoid announcing invalid endpoint again.
        bool splitTxtRecord = true;
        //If the service is local only,then its txt record must be added all at once
        if (entry->ifIndex == kDNSServiceInterfaceIndexLocalOnly) {
            splitTxtRecord = false;
        }
        char txtBuf[DZC_MAX_TXT_RECORD_SIZE] = {0};
        TXTRecordRef txtRecord;
        celix_properties_iterator_t propIter = celix_properties_begin(entry->properties);

        TXTRecordCreate(&txtRecord, sizeof(txtBuf), txtBuf);
        (void)TXTRecordSetValue(&txtRecord, DZC_TXT_RECORD_VERSION_KEY, sizeof(DZC_CURRENT_TXT_RECORD_VERSION)-1, DZC_CURRENT_TXT_RECORD_VERSION);
        char propSizeStr[16]= {0};
        sprintf(propSizeStr, "%zu", celix_properties_size(entry->properties) + 2/*size and version*/);
        (void)TXTRecordSetValue(&txtRecord, DZC_SERVICE_PROPERTIES_SIZE_KEY, strlen(propSizeStr), propSizeStr);
        if (!discoveryZeroconfAnnouncer_copyPropertiesToTxtRecord(announcer, &propIter, &txtRecord, sizeof(txtBuf), splitTxtRecord)) {
            TXTRecordDeallocate(&txtRecord);
            continue;
        }

        DNSServiceErrorType dnsErr;
        char instanceName[64] = {0};
        DNSServiceRef dsRef = announcer->sharedRef;//DNSServiceRegister will set a new value for dsRef
        int bytes = 0;
        if (entry->conflictCnt <= 1) {
            bytes = snprintf(instanceName, sizeof(instanceName), "%s-%ld", entry->serviceName, (long)announcer->pid);
        } else {
            bytes = snprintf(instanceName, sizeof(instanceName), "%s-%ld(%u)", entry->serviceName, (long)announcer->pid, entry->conflictCnt);
        }
        if (bytes >= sizeof(instanceName)) {
            celix_logHelper_error(announcer->logHelper, "Announcer: Please reduce the length of service name for %s.", entry->serviceName);
            TXTRecordDeallocate(&txtRecord);
            continue;
        }
        celix_logHelper_info(announcer->logHelper, "Announcer: Register service %s on interface %d.", instanceName, entry->ifIndex);
        dnsErr = DNSServiceRegister(&dsRef, kDNSServiceFlagsShareConnection, entry->ifIndex, instanceName, entry->serviceType, "local", NULL, htons(entry->port), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), OnDNSServiceRegisterCallback, announcer);
        if (dnsErr != kDNSServiceErr_NoError) {
            celix_logHelper_error(announcer->logHelper, "Announcer: Failed to announce service, %s. %d", instanceName, dnsErr);
            TXTRecordDeallocate(&txtRecord);
            continue;
        }
        TXTRecordDeallocate(&txtRecord);

        entry->registerRef = dsRef;
        while (!celix_propertiesIterator_isEnd(&propIter)) {
            TXTRecordCreate(&txtRecord, sizeof(txtBuf), txtBuf);
            if (!discoveryZeroconfAnnouncer_copyPropertiesToTxtRecord(announcer, &propIter, &txtRecord, sizeof(txtBuf), true)) {
                TXTRecordDeallocate(&txtRecord);
                break;
            }
            DNSRecordRef rdRef;//It will be free when deallocate dsRef
            dnsErr = DNSServiceAddRecord(dsRef, &rdRef, 0, kDNSServiceType_TXT, TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), 0);
            if (dnsErr != kDNSServiceErr_NoError) {
                celix_logHelper_error(announcer->logHelper, "Announcer: Failed to add record for %s. %d", instanceName, dnsErr);
                TXTRecordDeallocate(&txtRecord);
                break;
            }
            TXTRecordDeallocate(&txtRecord);
        }
    }
    return;
}

static void discoveryZeroconfAnnouncer_handleMDNSEvent(discovery_zeroconf_announcer_t *announcer) {
    DNSServiceErrorType dnsErr = DNSServiceProcessResult(announcer->sharedRef);
    if (dnsErr == kDNSServiceErr_ServiceNotRunning || dnsErr == kDNSServiceErr_DefunctConnection) {

        celix_logHelper_error(announcer->logHelper, "Announcer: mDNS connection may be broken, %d.", dnsErr);

        DNSServiceRefDeallocate(announcer->sharedRef);
        announcer->sharedRef = NULL;

        announce_endpoint_entry_t *entry;
        celixThreadMutex_lock(&announcer->mutex);
        CELIX_STRING_HASH_MAP_ITERATE(announcer->endpoints, iter) {
            entry = (announce_endpoint_entry_t *) iter.value.ptrValue;
            entry->registerRef = NULL;//no need free entry->registerRef, 'DNSServiceRefDeallocate(announcer->sharedRef)' has done it.
            entry->announced = false;
        }
        int size = celix_arrayList_size(announcer->revokedEndpoints);
        for (int i = 0; i < size; ++i) {
            entry = celix_arrayList_get(announcer->revokedEndpoints, i);
            entry->registerRef = NULL;//no need free entry->registerRef, 'DNSServiceRefDeallocate(announcer->sharedRef)' has done it.
        }
        celixThreadMutex_unlock(&announcer->mutex);
    } else if (dnsErr != kDNSServiceErr_NoError) {
        celix_logHelper_error(announcer->logHelper, "Announcer: Failed to process mDNS result, %d.", dnsErr);
    }
    return;
}

static void *discoveryZeroconfAnnouncer_refreshEndpointThread(void *data) {
    discovery_zeroconf_announcer_t *announcer = (discovery_zeroconf_announcer_t *)data;
    DNSServiceErrorType dnsErr;
    celix_array_list_t *announcedEndpoints = celix_arrayList_create();
    assert(announcedEndpoints != NULL);
    celix_array_list_t *revokedEndpoints = celix_arrayList_create();
    assert(revokedEndpoints != NULL);
    fd_set readfds;
    eventfd_t val;
    int dsFd;
    int maxFd;
    struct timeval *timeout = NULL;
    struct timeval timeVal;
    bool running = announcer->running;
    while (running) {
        if (announcer->sharedRef == NULL) {
            dnsErr = DNSServiceCreateConnection(&announcer->sharedRef);
            if (dnsErr == kDNSServiceErr_NoError) {
                discoveryZeroconfAnnouncer_eventNotify(announcer);// Trigger an event to announce all existing endpoints
            } else {
                celix_logHelper_error(announcer->logHelper, "Announcer: Failed to create connection for DNS service, %d.", dnsErr);
            }
        }

        FD_ZERO(&readfds);
        FD_SET(announcer->eventFd, &readfds);
        maxFd = announcer->eventFd;
        if (announcer->sharedRef) {
            dsFd = DNSServiceRefSockFD(announcer->sharedRef);
            assert(dsFd >= 0);
            FD_SET(dsFd, &readfds);
            maxFd = MAX(maxFd, dsFd);
            timeout = NULL;
        } else {
            dsFd = -1;
            timeVal.tv_sec = 5;//If the connection fails to be created, reconnect it after 5 seconds
            timeVal.tv_usec = 0;
            timeout = &timeVal;
        }

        int result = select(maxFd+1, &readfds, NULL, NULL, timeout);
        if (result > 0) {
            if (FD_ISSET(announcer->eventFd, &readfds)) {
                eventfd_read(announcer->eventFd, &val);

                celixThreadMutex_lock(&announcer->mutex);
                int size = celix_arrayList_size(announcer->revokedEndpoints);
                for (int i = 0; i < size; ++i) {
                    celix_arrayList_add(revokedEndpoints, celix_arrayList_get(announcer->revokedEndpoints, i));
                }
                celix_arrayList_clear(announcer->revokedEndpoints);

                if (announcer->sharedRef != NULL) {
                    CELIX_STRING_HASH_MAP_ITERATE(announcer->endpoints, iter) {
                        announce_endpoint_entry_t *entry = (announce_endpoint_entry_t *) iter.value.ptrValue;
                        if (entry->announced == false && entry->registerRef == NULL) {
                            celix_arrayList_add(announcedEndpoints, entry);
                        }
                    }
                }
                celixThreadMutex_unlock(&announcer->mutex);

                discoveryZeroconfAnnouncer_revokeEndpoints(announcer, revokedEndpoints);
                discoveryZeroconfAnnouncer_announceEndpoints(announcer, announcedEndpoints);
                celix_arrayList_clear(revokedEndpoints);
                celix_arrayList_clear(announcedEndpoints);
            }

            if (dsFd >= 0 && FD_ISSET(dsFd, &readfds)) {
                discoveryZeroconfAnnouncer_handleMDNSEvent(announcer);
            }
        } else if (result == -1 && errno != EINTR) {
            celix_logHelper_error(announcer->logHelper, "Announcer: Error Selecting event, %d.", errno);
            sleep(1);//avoid busy loop
        }
        celixThreadMutex_lock(&announcer->mutex);
        running = announcer->running;
        celixThreadMutex_unlock(&announcer->mutex);
    }
    if (announcer->sharedRef) {
        DNSServiceRefDeallocate(announcer->sharedRef);
    }
    celix_arrayList_destroy(revokedEndpoints);
    celix_arrayList_destroy(announcedEndpoints);
    return NULL;
}
