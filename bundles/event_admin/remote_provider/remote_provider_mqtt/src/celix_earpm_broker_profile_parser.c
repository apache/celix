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

#include "celix_earpm_broker_profile_parser.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#ifdef __linux__
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <fcntl.h>
#endif //__linux__

#include "celix_compiler.h"
#include "celix_cleanup.h"
#include "celix_stdlib_cleanup.h"
#include "celix_stdio_cleanup.h"
#include "celix_unistd_cleanup.h"
#include "celix_string_hash_map.h"
#include "celix_threads.h"
#include "celix_utils.h"
#include "celix_mqtt_broker_info_service.h"
#include "celix_earpm_constants.h"

typedef struct celix_earpmp_broker_service_entry {
    long serviceId;
    celix_mqtt_broker_info_service_t brokerInfoService;
    bool inactive;
    celix_bundle_context_t* ctx;
}celix_earpmp_broker_service_entry_t;

typedef struct celix_earpmp_broker_listener {
    char* host;
    uint16_t port;
    int family;
    char* bindInterface;
}celix_earpmp_broker_listener_t;

struct celix_earpm_broker_profile_parser {
    celix_bundle_context_t* ctx;
    celix_log_helper_t* logHelper;
    const char* brokerProfilePath;
    celix_string_hash_map_t* brokerInfoServices;//key:"address:port" val:celix_earpm_broker_service_entry_t*
    celix_array_list_t* brokerListeners;//element:celix_earpmb_broker_listener_t*
    celix_thread_t workerThread;
    bool running;
    int ifNotifySocket;
    int socketPair[2];//Used to wake up the parser thread
};

static void celix_earpmp_brokerInfoServiceEntryDestroy(celix_earpmp_broker_service_entry_t* entry);
static void celix_earpmp_brokerListenerDestroy(celix_earpmp_broker_listener_t* listener);
static int celix_earpmp_openIfNotifySocket(void);
static void* celix_earpmp_thread(void* data);

celix_earpm_broker_profile_parser_t* celix_earpmp_create(celix_bundle_context_t* ctx) {
    assert(ctx != NULL);
    celix_autofree celix_earpm_broker_profile_parser_t* parser = calloc(1, sizeof(*parser));
    if (parser == NULL) {
        return NULL;
    }
    parser->ctx = ctx;
    parser->brokerProfilePath = celix_bundleContext_getProperty(ctx, CELIX_EARPM_BROKER_PROFILE, CELIX_EARPM_BROKER_PROFILE_DEFAULT);

    celix_autoptr(celix_log_helper_t) logHelper = parser->logHelper = celix_logHelper_create(ctx, "CELIX_EARPM_BPP");
    if (logHelper == NULL) {
        return NULL;
    }

    celix_autoptr(celix_string_hash_map_t) brokerInfoServices = NULL;
    {
        celix_string_hash_map_create_options_t opts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
        opts.simpleRemovedCallback = (void *) celix_earpmp_brokerInfoServiceEntryDestroy;
        brokerInfoServices = parser->brokerInfoServices = celix_stringHashMap_createWithOptions(&opts);
        if (brokerInfoServices == NULL) {
            celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(logHelper, "Failed to create broker info services map.");
            return NULL;
        }
    }

    celix_autoptr(celix_array_list_t) brokerListeners = NULL;
    {
        celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
        opts.elementType = CELIX_ARRAY_LIST_ELEMENT_TYPE_POINTER;
        opts.simpleRemovedCallback = (void *) celix_earpmp_brokerListenerDestroy;
        brokerListeners = parser->brokerListeners = celix_arrayList_createWithOptions(&opts);
        if (brokerListeners == NULL) {
            celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(logHelper, "Failed to create broker listeners list.");
            return NULL;
        }
    }

    celix_auto(celix_fd_t) ifNotifySocket = parser->ifNotifySocket = celix_earpmp_openIfNotifySocket();
    if (ifNotifySocket < 0) {
        celix_logHelper_warning(logHelper, "Failed to open if notify socket, maybe not supported. Dynamic ip will be disabled.");
    }
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, parser->socketPair) != 0) {
        celix_logHelper_error(logHelper, "Failed to create socket pair for thread communication.");
        return NULL;
    }
    celix_auto(celix_fd_t) socketPairRead = parser->socketPair[0];
    celix_auto(celix_fd_t) socketPairWrite = parser->socketPair[1];

    parser->running = true;
    celix_status_t status = celixThread_create(&parser->workerThread, NULL, celix_earpmp_thread, parser);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to create broker profile parser thread.");
        return NULL;
    }

    celix_steal_fd(&socketPairRead);
    celix_steal_fd(&socketPairWrite);
    celix_steal_fd(&ifNotifySocket);
    celix_steal_ptr(brokerListeners);
    celix_steal_ptr(brokerInfoServices);
    celix_steal_ptr(logHelper);

    return celix_steal_ptr(parser);
}

void celix_earpmp_destroy(celix_earpm_broker_profile_parser_t* parser) {
    __atomic_store_n(&parser->running, false, __ATOMIC_RELEASE);
    char c = 0;
    (void)write(parser->socketPair[1], &c, 1);//wakeup worker thread
    celixThread_join(parser->workerThread, NULL);
    close(parser->socketPair[0]);
    close(parser->socketPair[1]);
    close(parser->ifNotifySocket);
    celix_arrayList_destroy(parser->brokerListeners);
    celix_stringHashMap_destroy(parser->brokerInfoServices);
    celix_logHelper_destroy(parser->logHelper);
    free(parser);
}

static int celix_earpmp_openIfNotifySocket(void) {
#ifdef __linux__
    celix_auto(celix_fd_t) fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) {
        return -1;
    }
    (void)fcntl(fd, F_SETFL, O_NONBLOCK);
    struct sockaddr_nl snl;
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
    int rc = bind(fd, (struct sockaddr *) &snl, sizeof snl);
    if (rc != 0) {
        return -1;
    }

    return celix_steal_fd(&fd);
#else
    return -1;//Not supported
#endif
}

static celix_earpmp_broker_listener_t* celix_earpmp_brokerListenerCreate(const char* host, uint16_t port) {
    celix_autofree celix_earpmp_broker_listener_t* listener = calloc(1, sizeof(*listener));
    if (listener == NULL) {
        return NULL;
    }
    listener->port = port;
    listener->family = AF_UNSPEC;
    listener->bindInterface = NULL;
    listener->host = NULL;
    if (host) {
        listener->host = celix_utils_strdup(host);
        if (listener->host == NULL) {
            return NULL;
        }
    }
    return celix_steal_ptr(listener);
}

static void celix_earpmp_brokerListenerDestroy(celix_earpmp_broker_listener_t* listener) {
    free(listener->host);
    free(listener->bindInterface);
    free(listener);
    return;
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpmp_broker_listener_t, celix_earpmp_brokerListenerDestroy)

static void celix_earpmp_stripLine(char* line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[len - 1] = '\0';
        len = strlen(line);
    }
    return ;
}

static bool celix_earpmp_parseBrokerProfile(celix_earpm_broker_profile_parser_t* parser, FILE* file) {
    char line[512] = {0};
    char* token = NULL;
    char* save = NULL;
    celix_earpmp_broker_listener_t* curListener = NULL;
    while (fgets(line, sizeof(line), file) != NULL) {
        if(line[0] == '#' || line[0] == '\n' || line[0] == '\r'){
            continue;
        }
        celix_earpmp_stripLine(line);

        token = strtok_r(line, " ", &save);
        if (token == NULL) {
            continue;
        }
        if (celix_utils_stringEquals(token, "listener")) {
            curListener = NULL;
            token = strtok_r(NULL, " ", &save);
            if (token == NULL) {
                celix_logHelper_error(parser->logHelper, "Invalid listener line in broker profile file %s.", parser->brokerProfilePath);
                continue;
            }
            char* portEnd = NULL;
            long port = strtol(token, &portEnd, 10);
            if (portEnd == NULL || *portEnd != '\0' || portEnd == token || port < 0 || port > UINT16_MAX) {
                celix_logHelper_error(parser->logHelper, "Invalid port in listener line(%s) in broker profile file %s.", token, parser->brokerProfilePath);
                continue;
            }
            char* host = strtok_r(NULL, " ", &save);
            if (port == 0 && host == NULL) {
                celix_logHelper_error(parser->logHelper, "Invalid unix socket listener line in broker profile file %s.", parser->brokerProfilePath);
                continue;
            }
            celix_autoptr(celix_earpmp_broker_listener_t) listener = celix_earpmp_brokerListenerCreate(host,(uint16_t) port);
            if (listener == NULL) {
                celix_logHelper_error(parser->logHelper, "Failed to create broker listener for port %ld.", port);
                continue;
            }
            celix_status_t status = celix_arrayList_add(parser->brokerListeners, listener);
            if (status != CELIX_SUCCESS) {
                celix_logHelper_error(parser->logHelper, "Failed to add broker listener for port %ld.", port);
                continue;
            }
            curListener = celix_steal_ptr(listener);
        } else if (celix_utils_stringEquals(token, "socket_domain") && curListener != NULL) {
            token = strtok_r(NULL, " ", &save);
            if (token == NULL) {
                continue;
            }
            if (celix_utils_stringEquals(token, "ipv4")) {
                curListener->family = AF_INET;
            } else if (celix_utils_stringEquals(token, "ipv6")) {
                curListener->family = AF_INET6;
            }
        } else if (celix_utils_stringEquals(token, "bind_interface") && curListener != NULL) {
            token = strtok_r(NULL, " ", &save);
            if (token == NULL) {
                continue;
            }
            curListener->bindInterface = celix_utils_strdup(token);
            if (curListener->bindInterface == NULL) {
                celix_logHelper_error(parser->logHelper, "Failed to dup bind interface %s.", token);
                continue;
            }
        }
    }
    return celix_arrayList_size(parser->brokerListeners) > 0;
}

static celix_status_t celix_earpmp_loadBrokerProfile(celix_earpm_broker_profile_parser_t* parser) {
    while (__atomic_load_n(&parser->running, __ATOMIC_ACQUIRE)) {
        celix_autoptr(FILE) file = fopen(parser->brokerProfilePath, "r");
        if (file == NULL && errno == ENOENT) {
            sleep(1);
            continue;
        } else if (file == NULL) {
            celix_logHelper_error(parser->logHelper, "Failed to open broker profile file %s. %d.", parser->brokerProfilePath, errno);
            return CELIX_FILE_IO_EXCEPTION;
        }
        if (celix_earpmp_parseBrokerProfile(parser, file) == false) {
            celix_logHelper_error(parser->logHelper, "Failed to parse broker profile file %s.", parser->brokerProfilePath);
            return CELIX_INVALID_SYNTAX;
        }
        return CELIX_SUCCESS;
    }
    return CELIX_ILLEGAL_STATE;
}

static celix_earpmp_broker_service_entry_t* celix_earpmp_brokerInfoServiceEntryCreate(celix_earpm_broker_profile_parser_t* parser, const char* host, int port) {
    celix_autofree celix_earpmp_broker_service_entry_t* entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        celix_logHelper_error(parser->logHelper, "Failed to create broker info service entry.");
        return NULL;
    }
    entry->ctx = parser->ctx;
    entry->inactive = false;
    entry->brokerInfoService.handle = NULL;
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.serviceName = CELIX_MQTT_BROKER_INFO_SERVICE_NAME;
    opts.serviceVersion = CELIX_MQTT_BROKER_INFO_SERVICE_VERSION;
    opts.svc = &entry->brokerInfoService;
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    if (props == NULL) {
        celix_logHelper_logTssErrors(parser->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(parser->logHelper, "Failed to create properties for broker info service.");
        return NULL;
    }
    celix_status_t status = celix_properties_setString(props, CELIX_MQTT_BROKER_ADDRESS, host);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(parser->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(parser->logHelper, "Failed to set addresses property for broker info service.");
        return NULL;
    }
    status = celix_properties_setLong(props, CELIX_MQTT_BROKER_PORT, port);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(parser->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(parser->logHelper, "Failed to set port property for broker info service.");
        return NULL;
    }
    status = celix_properties_setString(props, "service.exported.interfaces", "*");
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(parser->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(parser->logHelper, "Failed to set exported interfaces property for broker info service.");
        return NULL;
    }
    opts.properties = celix_steal_ptr(props);
    entry->serviceId = celix_bundleContext_registerServiceWithOptionsAsync(parser->ctx, &opts);
    if (entry->serviceId < 0) {
        celix_logHelper_error(parser->logHelper, "Failed to register broker info service.");
        return NULL;
    }
    return celix_steal_ptr(entry);
}

static void celix_earpmp_brokerInfoServiceEntryDestroy(celix_earpmp_broker_service_entry_t* entry) {
    return celix_bundleContext_unregisterServiceAsync(entry->ctx, entry->serviceId, entry, free);
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpmp_broker_service_entry_t, celix_earpmp_brokerInfoServiceEntryDestroy)

static void celix_earpmp_addBrokerInfoService(celix_earpm_broker_profile_parser_t* parser, const char*host, int port) {
    char brokerInfoSvcKey[512] = {0};//It is enough to store the key "host/ip:port"
    (void)snprintf(brokerInfoSvcKey, sizeof(brokerInfoSvcKey), "%s:%d", host, port);
    celix_earpmp_broker_service_entry_t* serviceEntry = celix_stringHashMap_get(parser->brokerInfoServices, brokerInfoSvcKey);
    if (serviceEntry != NULL) {
        serviceEntry->inactive = false;
        return;
    }
    serviceEntry = celix_earpmp_brokerInfoServiceEntryCreate(parser, host, port);
    if (serviceEntry == NULL) {
        return;
    }
    celix_status_t status = celix_stringHashMap_put(parser->brokerInfoServices, brokerInfoSvcKey, serviceEntry);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(parser->logHelper, "Failed to register broker info service for %s:%d.", host, port);
        return;
    }
    return;
}

static void celix_earpmp_addBrokerInfoServicesForBindInterfacesListener(celix_earpm_broker_profile_parser_t* parser, celix_earpmp_broker_listener_t* listener, struct ifaddrs* ifAddresses) {
    for (struct ifaddrs* ifa = ifAddresses; ifa!=NULL; ifa=ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        if (listener->bindInterface == NULL /*bind all interfaces*/|| celix_utils_stringEquals(ifa->ifa_name, listener->bindInterface)) {
            char host[INET6_ADDRSTRLEN] = {0};
            const char* result = NULL;
            if (ifa->ifa_addr->sa_family == AF_INET && (listener->family == AF_INET || listener->family == AF_UNSPEC)) {
                result = inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, host, sizeof(host));
            } else if (ifa->ifa_addr->sa_family == AF_INET6 && (listener->family == AF_INET6 || listener->family == AF_UNSPEC)) {
                result = inet_ntop(AF_INET6, &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr, host, sizeof(host));
            } else {
                continue;
            }
            if (result == NULL) {
                celix_logHelper_error(parser->logHelper, "Failed to convert ip address to string for interface %s.", ifa->ifa_name);
                continue;
            }
            celix_earpmp_addBrokerInfoService(parser, host, listener->port);
        }
    }
    return;
}

static void celix_earpmp_updateBrokerInfoServices(celix_earpm_broker_profile_parser_t* parser, struct ifaddrs* ifAddresses) {
    struct ifaddrs* _ifAddresses = NULL;
    if (ifAddresses == NULL) {
        if(getifaddrs(&_ifAddresses) < 0) {
            celix_logHelper_warning(parser->logHelper, "Failed to get interface addresses.");
        }
        ifAddresses = _ifAddresses;
    }
    size_t size = celix_arrayList_size(parser->brokerListeners);
    for (int i = 0; i < size; ++i) {
        celix_earpmp_broker_listener_t* listener = celix_arrayList_get(parser->brokerListeners, i);
        if (listener->host != NULL) {
            celix_earpmp_addBrokerInfoService(parser, listener->host, listener->port);
        } else {
            celix_earpmp_addBrokerInfoServicesForBindInterfacesListener(parser, listener, ifAddresses);
        }
    }
    if (_ifAddresses != NULL) {
        freeifaddrs(_ifAddresses);
    }
    return;
}

static void celix_earpmp_markAllBrokerInfoServicesInactive(celix_earpm_broker_profile_parser_t* parser) {
    CELIX_STRING_HASH_MAP_ITERATE(parser->brokerInfoServices, iter) {
        celix_earpmp_broker_service_entry_t* entry = iter.value.ptrValue;
        entry->inactive = true;
    }
    return;
}

static void celix_earpmp_removeAllInactiveBrokerInfoServices(celix_earpm_broker_profile_parser_t* parser) {
    celix_string_hash_map_iterator_t iter = celix_stringHashMap_begin(parser->brokerInfoServices);
    while (!celix_stringHashMapIterator_isEnd(&iter)) {
        celix_earpmp_broker_service_entry_t* entry = iter.value.ptrValue;
        if (entry->inactive) {
            celix_stringHashMapIterator_remove(&iter);
        } else {
            celix_stringHashMapIterator_next(&iter);
        }
    }
    return;
}

#ifdef __linux__
static bool celix_earpmp_isInterfacesChanged(int ifNotifySocket) {
    char buff[4096] = {0};
    struct nlmsghdr *nlMsg = (struct nlmsghdr*) buff;
    ssize_t readCount = read(ifNotifySocket, buff, sizeof buff);
    while (1) {
        //Read the netlink message
        if (((char*)&nlMsg[1] > (buff + readCount)) || ((char*)nlMsg + nlMsg->nlmsg_len > (buff + readCount))) {
            if (buff < (char*) nlMsg) {
                // discard processed data
                readCount -= ((char*) nlMsg - buff);
                memmove(buff, nlMsg, readCount);
                nlMsg = (struct nlmsghdr*) buff;
                // read more data
                readCount += read(ifNotifySocket, buff + readCount, sizeof(buff) - readCount);
                continue;
            } else {
                break;  // Message is too long to store in buffer
            }
        }

        // Process the netlink message
        if (nlMsg->nlmsg_type == RTM_DELLINK || nlMsg->nlmsg_type == RTM_NEWLINK
            || nlMsg->nlmsg_type == RTM_DELADDR || nlMsg->nlmsg_type == RTM_NEWADDR) {
            return true;
        }

        // Get the next message in the buffer
        if ((nlMsg->nlmsg_flags & NLM_F_MULTI) != 0 && nlMsg->nlmsg_type != NLMSG_DONE) {
            ssize_t len = readCount - ((char*)nlMsg - buff);
            nlMsg = NLMSG_NEXT(nlMsg, len);
        } else {
            break;  // All done
        }
    }
    return false;
}
#else //__linux__
static bool celix_earpmp_isInterfacesChanged(int ifNotifySocket)  {
    //Not supported
    return false;
}
#endif //__linux__

static void celix_earpmp_interfacesChangedDoNext(celix_earpm_broker_profile_parser_t* parser) {
    struct ifaddrs* ifAddresses = NULL;
    if(getifaddrs(&ifAddresses) < 0) {
        celix_logHelper_error(parser->logHelper, "Failed to get interface addresses.");
        return;
    }
    celix_earpmp_markAllBrokerInfoServicesInactive(parser);
    celix_earpmp_updateBrokerInfoServices(parser, ifAddresses);
    celix_earpmp_removeAllInactiveBrokerInfoServices(parser);
    freeifaddrs(ifAddresses);
    return;
}

static void* celix_earpmp_thread(void* data) {
    assert(data != NULL);
    celix_earpm_broker_profile_parser_t* parser = data;
    celix_logHelper_trace(parser->logHelper, "Broker profile parser thread started.");

    celix_status_t status = celix_earpmp_loadBrokerProfile(parser);
    if (status != CELIX_SUCCESS) {
        return NULL;
    }
    celix_earpmp_updateBrokerInfoServices(parser, NULL);

    while (__atomic_load_n(&parser->running, __ATOMIC_ACQUIRE)) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int maxfd = -1;
        if (parser->ifNotifySocket >= 0) {
            FD_SET(parser->ifNotifySocket, &readfds);
            maxfd = parser->ifNotifySocket;
        }

        FD_SET(parser->socketPair[0], &readfds);
        maxfd = parser->socketPair[0] > maxfd ? parser->socketPair[0] : maxfd;
        int rc = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (rc < 0) {
            if (errno != EINTR) {
                celix_logHelper_error(parser->logHelper, "Failed to select. %d.", errno);
                sleep(1);
            }
            continue;
        }
        if (parser->ifNotifySocket >= 0 && FD_ISSET(parser->ifNotifySocket, &readfds)) {
            if (celix_earpmp_isInterfacesChanged(parser->ifNotifySocket)) {
                celix_earpmp_interfacesChangedDoNext(parser);
            }
        }
        if (FD_ISSET(parser->socketPair[0], &readfds)) {
            char buf[1] = {0};
            (void)read(parser->socketPair[0], buf, sizeof(buf));
        }
    }

    celix_logHelper_trace(parser->logHelper, "Broker profile parser thread stopped.");
    return NULL;
}