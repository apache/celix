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
/**
 * ip_utils.c
 *
 *  \date       Jun 24, 2019
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "ip_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <unistd.h>

unsigned int ipUtils_ipToUnsignedInt(char *ip) {
    unsigned int ipAsUint = 0;

    char *partOfIp = NULL, *savePtr = NULL;
    char *input = strdup(ip); // Make a copy because otherwise strtok_r manipulates the input string
    partOfIp = strtok_r(input, ".\0", &savePtr); ipAsUint += strtoul(partOfIp, NULL, 10) * (unsigned int) pow(256, 3);
    partOfIp = strtok_r(NULL, ".\0", &savePtr);  ipAsUint += strtoul(partOfIp, NULL, 10) * (unsigned int) pow(256, 2);
    partOfIp = strtok_r(NULL, ".\0", &savePtr);  ipAsUint += strtoul(partOfIp, NULL, 10) * (unsigned int) pow(256, 1);
    partOfIp = strtok_r(NULL, ".\0", &savePtr);  ipAsUint += strtoul(partOfIp, NULL, 10) * (unsigned int) pow(256, 0);
    free(input);

    return ipAsUint;
}

char *ipUtils_unsignedIntToIp(unsigned int ip) {
    char *ipStr = calloc(16, sizeof(char));

    int ipPart1 = ip / (int) pow(256, 3); ip -= ipPart1 * (int) pow(256, 3);
    int ipPart2 = ip / (int) pow(256, 2); ip -= ipPart2 * (int) pow(256, 2);
    int ipPart3 = ip / (int) pow(256, 1); ip -= ipPart3 * (int) pow(256, 1);
    int ipPart4 = ip / (int) pow(256, 0);

    snprintf(ipStr, 16, "%d.%d.%d.%d", ipPart1, ipPart2, ipPart3, ipPart4);

    return ipStr;
}

unsigned int ipUtils_prefixToBitmask(unsigned int prefix) {
    return (0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF;
}

int ipUtils_netmaskToPrefix(const char *netmask) {
    // Convert netmask to in_addr object
    struct in_addr in;
    int ret = inet_pton(AF_INET, netmask, &in);
    if (ret == 0)
        return -1;

    // Now convert the mask to a prefix
    int prefix = 0;
    bool processed_one = false;
    unsigned int i = ntohl(in.s_addr);

    while (i > 0) {
        if (i & 1) {
            prefix++;
            processed_one = true;
        } else {
            if (processed_one) return -1;
        }

        i >>= 1;
    }

    return prefix;
}

/** Finds an IP of the available network interfaces of the machine by specifying an CIDR subnet.
 *
 * @param ipWithPrefix  IP with prefix, e.g. 192.168.1.0/24
 * @return ip           In case a matching interface could be found, an allocated string containing the IP of the
 *                      interface will be returned, e.g. 192.168.1.16. Memory for the new string can be freed with free().
 *                      When no matching interface is found NULL will be returned.
 */
char *ipUtils_findIpBySubnet(const char *ipWithPrefix) {
    char *ip = NULL;

    char *input = strndup(ipWithPrefix, 19); // Make a copy as otherwise strtok_r manipulates the input string

    char *savePtr;
    char *inputIp = strtok_r(input, "/", &savePtr);
    char *inputPrefixStr = strtok_r(NULL, "\0", &savePtr);
    unsigned int inputPrefix = (unsigned int) strtoul(inputPrefixStr, NULL, 10);

    unsigned int ipAsUint = ipUtils_ipToUnsignedInt(inputIp);
    unsigned int bitmask = ipUtils_prefixToBitmask(inputPrefix);

    unsigned int ipRangeStart = ipAsUint & bitmask;
    unsigned int ipRangeStop = ipAsUint | ~bitmask;

    // Requested IP range is known now, now loop through network interfaces
    struct ifaddrs *ifap, *ifa;

    getifaddrs (&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        if (ifa->ifa_addr->sa_family != AF_INET)
            continue;

        // Retrieve IP address for interface
        char if_addr[NI_MAXHOST];
        int rv = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                             if_addr, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if (rv != 0) {
            printf("getnameinfo() failed: %s\n", gai_strerror(rv));
            continue;
        }

        // Retrieve netmask
        struct sockaddr_in *sa = (struct sockaddr_in *) ifa->ifa_netmask;
        char *if_netmask = inet_ntoa(sa->sin_addr);

        unsigned int ifIpAsUint = ipUtils_ipToUnsignedInt(if_addr);
        int ifPrefix = ipUtils_netmaskToPrefix(if_netmask);
        if (ifPrefix == -1) {
            break;
        }

        if (ifIpAsUint >= ipRangeStart && ifIpAsUint <= ipRangeStop && inputPrefix >= ifPrefix) {
            ip = strndup(if_addr, 1024);
            break;
        }
    }
    freeifaddrs(ifap);
    free(input);

    return ip;
}
