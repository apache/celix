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

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "celix_log.h"
#include "constants.h"

#include <curl/curl.h>
#include "etcd.h"
#include "etcd_watcher.h"

#include "pubsub_discovery.h"
#include "pubsub_discovery_impl.h"


#define MAX_ROOTNODE_LENGTH		128
#define MAX_LOCALNODE_LENGTH 	4096
#define MAX_FIELD_LENGTH		128

#define CFG_ETCD_SERVER_IP		"PUBSUB_DISCOVERY_ETCD_SERVER_IP"
#define DEFAULT_ETCD_SERVER_IP	"127.0.0.1"

#define CFG_ETCD_SERVER_PORT	"PUBSUB_DISCOVERY_ETCD_SERVER_PORT"
#define DEFAULT_ETCD_SERVER_PORT 2379

// be careful - this should be higher than the curl timeout
#define CFG_ETCD_TTL   "DISCOVERY_ETCD_TTL"
#define DEFAULT_ETCD_TTL 30


celix_status_t etcdCommon_init(bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    const char* etcd_server = NULL;
    const char* etcd_port_string = NULL;
    int etcd_port = 0;

    if ((bundleContext_getProperty(context, CFG_ETCD_SERVER_IP, &etcd_server) != CELIX_SUCCESS) || !etcd_server) {
        etcd_server = DEFAULT_ETCD_SERVER_IP;
    }

    if ((bundleContext_getProperty(context, CFG_ETCD_SERVER_PORT, &etcd_port_string) != CELIX_SUCCESS) || !etcd_port_string) {
        etcd_port = DEFAULT_ETCD_SERVER_PORT;
    } else {
        char* endptr = NULL;
        errno = 0;
        etcd_port = strtol(etcd_port_string, &endptr, 10);
        if (*endptr || errno != 0) {
            etcd_port = DEFAULT_ETCD_SERVER_PORT;
        }
    }

    printf("PSD: Using discovery HOST:PORT: %s:%i\n", etcd_server, etcd_port);

    if (etcd_init(etcd_server, etcd_port, CURL_GLOBAL_DEFAULT) != 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        status = CELIX_SUCCESS;
    }

    return status;
}

