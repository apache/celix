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

#include "celix_ip_utils.h"

#include "celix_err.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"
#include "celix_convert_utils.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <ifaddrs.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

uint32_t celix_utils_convertIpToUint(const char* ip, bool* converted) {
    if (converted) {
        *converted = false;
    }

    // copy for strtok_r
    celix_autofree char* input = celix_utils_strdup(ip);
    if (!input) {
        celix_err_push("Failed to duplicate input string for IP conversion");
        return 0;
    }

    uint32_t ipAsUint = 0;
    char* partOfIp = NULL;
    char* savePtr = NULL;
    int count = 0;
    while ((partOfIp = strtok_r(partOfIp == NULL ? input : NULL, ".\0", &savePtr)) != NULL) {
        if (count > 3) {
            celix_err_pushf("Failed to convert IP address %s to unsigned int, to many parts", ip);
            return 0;
        }
        bool longConverted = false;
        long partAsLong = celix_utils_convertStringToLong(partOfIp, ULONG_MAX, &longConverted);
        if (!longConverted) {
            celix_err_pushf("Failed to convert IP address %s to unsigned int, part `%s` is not a number", ip, partOfIp);
            return 0;
        } else if (partAsLong > 255 || partAsLong < 0) {
            celix_err_pushf("Failed to convert IP address %s to unsigned int, part `%s` is out of range", ip, partOfIp);
            return 0;
        }
        ipAsUint += (uint32_t)(partAsLong * powl(256, 3 - count++));
    }

    if (converted) {
        *converted = true;
    }
    return ipAsUint;
}

char* celix_utils_convertUintToIp(uint32_t ip) {
    char* ipStr = calloc(16, sizeof(char));
    if (!ipStr) {
        celix_err_push("Failed to allocate memory for IP address string");
        return NULL;
    }

    int64_t ipPart1 = ip / (int64_t)pow(256, 3);
    ip -= ipPart1 * (int64_t)pow(256, 3);
    int64_t ipPart2 = ip / (int64_t)pow(256, 2);
    ip -= ipPart2 * (int64_t)pow(256, 2);
    int64_t ipPart3 = ip / (int64_t)pow(256, 1);
    ip -= ipPart3 * (int64_t)pow(256, 1);
    int64_t ipPart4 = ip / (int64_t)pow(256, 0);

    snprintf(ipStr, 16, "%li.%li.%li.%li", ipPart1, ipPart2, ipPart3, ipPart4);

    return ipStr;
}

uint32_t celix_utils_ipPrefixLengthToBitmask(int prefix) {
    if (prefix > 32 || prefix <= 0) {
        return 0;
    }
    return (uint32_t )((0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF);
}

int celix_utils_ipNetmaskToPrefixLength(const char* netmask) {
    // Convert netmask to in_addr object
    struct in_addr in;
    int ret = inet_pton(AF_INET, netmask, &in);
    if (ret != 1) {
        celix_err_pushf("Failed to convert netmask %s to in_addr object", netmask);
        return -1;
    }

    // Now convert the mask to a prefix
    int prefix = 0;
    uint32_t i = ntohl(in.s_addr);

    while (i > 0) {
        if (i & 1) {
            prefix++;
        }
        i >>= 1;
    }

    return prefix;
}

celix_status_t celix_utils_findIpInSubnet(const char* subnetCidrNotation, char** foundIp) {
    assert(subnetCidrNotation);
    assert(foundIp);

    *foundIp = NULL;

    //create copy for strtok_r
    celix_autofree char* input = celix_utils_strdup(subnetCidrNotation);
    if (!input) {
        celix_err_push("Failed to duplicate input string for subnet search");
        return CELIX_ENOMEM;
    }

    char* savePtr;
    char* inputIp = strtok_r(input, "/", &savePtr);
    char* inputPrefixStr = strtok_r(NULL, "\0", &savePtr);

    if (!inputPrefixStr) {
        celix_err_pushf("Failed to parse IP address with prefix %s. Missing a '/'", subnetCidrNotation);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    bool convertedLong = false;
    int inputPrefix = (int)celix_utils_convertStringToLong(inputPrefixStr, INT_MAX, &convertedLong);
    if (!convertedLong) {
        celix_err_pushf("Failed to parse prefix in IP address with prefix %s", subnetCidrNotation);
        return CELIX_ILLEGAL_ARGUMENT;
    } else if (inputPrefix > 32 || inputPrefix < 0) {
        celix_err_pushf(
            "Failed to parse IP address with prefix %s. Prefix %s is out of range", subnetCidrNotation, inputPrefixStr);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    bool converted;
    uint32_t ipAsUint = celix_utils_convertIpToUint(inputIp, &converted);
    if (!converted) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    uint32_t bitmask = celix_utils_ipPrefixLengthToBitmask(inputPrefix);

    unsigned int ipRangeStart = ipAsUint & bitmask;
    unsigned int ipRangeStop = ipAsUint | ~bitmask;

    // Requested IP range is known now, now loop through network interfaces
    struct ifaddrs *ifap, *ifa;

    if (getifaddrs(&ifap) == -1) {
        celix_status_t status = errno;
        celix_err_push("Failed to get network interfaces");
        return status;
    }

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }

        if (ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        // Retrieve IP address for interface
        char if_addr[NI_MAXHOST];
        int rv = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), if_addr, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if (rv != 0) {
            celix_err_pushf("Failed to get IP address for interface %s: %s", ifa->ifa_name, gai_strerror(rv));
            continue;
        }

        // Retrieve netmask
        struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_netmask;
        char* if_netmask = inet_ntoa(sa->sin_addr);

        uint32_t ifIpAsUint = celix_utils_convertIpToUint(if_addr, NULL);
        int ifPrefix = celix_utils_ipNetmaskToPrefixLength(if_netmask);
        if (ifPrefix == -1) {
            break;
        }

        if (ifIpAsUint >= ipRangeStart && ifIpAsUint <= ipRangeStop && inputPrefix >= ifPrefix) {
            char* ip = celix_utils_strdup(if_addr);
            if (!ip) {
                freeifaddrs(ifap);
                celix_err_push("Failed to duplicate IP address");
                return CELIX_ENOMEM;
            }
            *foundIp = ip;
            break;
        }
    }
    freeifaddrs(ifap);
    return CELIX_SUCCESS;
}
