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
#ifndef CELIX_DISCOVERY_ZEROCONF_CONSTANTS_H
#define CELIX_DISCOVERY_ZEROCONF_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <dns_sd.h>

#define DZC_SERVICE_ANNOUNCED_IF_INDEX_DEFAULT kDNSServiceInterfaceIndexLocalOnly

#define DZC_SERVICE_TYPE_KEY "DZC_SERVICE_TYPE_KEY"

#define DZC_SERVICE_PRIMARY_TYPE "_celix-rpc._udp"

#define DZC_HOST_DEFAULT "celix_rpc_dumb_host.local."
#define DZC_PORT_DEFAULT 50009

#define DZC_SERVICE_PROPERTIES_SIZE_KEY "DZC_SVC_PROPS_SIZE_KEY"

#ifdef __cplusplus
}
#endif

#endif //CELIX_DISCOVERY_ZEROCONF_CONSTANTS_H

