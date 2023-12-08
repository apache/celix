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

/**
 * The default network interface index for announcing service,
 * "kDNSServiceInterfaceIndexLocalOnly" indicates that service will be
 * discovered only by other local clients on the same machine.
 */
#define DZC_SERVICE_ANNOUNCED_IF_INDEX_DEFAULT kDNSServiceInterfaceIndexLocalOnly

/**
 * mDNS service name for celix service.
 * About mDNS service name, see rfc6763 section 4.1.2 and section 7
 */
#define DZC_SERVICE_PRIMARY_TYPE "_celix-rpc._udp"

/**
 * mDNS service subtype for celix.service, it can be null.
 * About mDNS service subtype, see rfc6763 section 7.1
 * The subtype mechanism can be illustrated with some examples using the dns-sd command-line tool:
 * If we register the following three services:
 *    % dns-sd -R service1 _celix-rpc._udp local 1001
 *    % dns-sd -R service2 _celix-rpc._udp,subtype1 local 1002
 *    % dns-sd -R service3   _celix-rpc._udp,subtype1,subtype2 local 1003
 * Now:
 *    % dns-sd -B _celix-rpc._udp             # will find all three services
 *    % dns-sd -B _celix-rpc._udp,subtype1 # will find "service2" and "service3"
 *    % dns-sd -B _celix-rpc._udp,subtype2 # will find only "service3"
 */
#define DZC_SERVICE_TYPE_KEY "DZC_SERVICE_TYPE_KEY"

/**
 * The default port for mDNS service.
 *
 * It is a dummy value, it is used for the remote service that IPC is not network based(eg:shared memory).
 */
#define DZC_PORT_DEFAULT 65535

/**
 * The size of celix service properties.
 *
 * Because we will split service properties into multi txt records,
 * we set the properties size to a txt records,
 * and use it to verify whether the complete service properties is resolved.
 */
#define DZC_SERVICE_PROPERTIES_SIZE_KEY "DZC_SVC_PROPS_SIZE_KEY"

#ifdef __cplusplus
}
#endif

#endif //CELIX_DISCOVERY_ZEROCONF_CONSTANTS_H

