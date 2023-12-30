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
 * @file celix_ip_utils.h
 * @brief Utility functions for IP address manipulation.
 *
 * This header file contains declarations of utility functions for
 * converting IP addresses and manipulating IP address data.
 */

#ifndef CELIX_IP_UTILS_H
#define CELIX_IP_UTILS_H

#include "celix_errno.h"
#include "celix_utils_export.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Converts an IP address in string format to an unsigned integer.
 *
 * This function takes an IP address as a string and converts it to
 * its corresponding unsigned integer representation.
 *
 * If the conversation failed, an error message is logged to celix_err.
 *
 * @param[in] ip The IP address in string format (e.g., "192.168.0.1").
 * @param[out] converted A boolean indicating whether the conversion was successful. Can be NULL.
 * @return The IP address as an uint32_t. Returns 0 if the conversion fails.
 */
uint32_t celix_utils_convertIpToUint(const char* ip, bool* converted);

/**
 * @brief Converts an unsigned integer to its corresponding IP address in string format.
 *
 * This function converts an unsigned integer representing an IP address
 * to a string format.
 *
 * @param[in] ip The IP address as an uint32_t.
 * @return The IP address in string format (e.g., "192.168.0.1").
 */
char* celix_utils_convertUintToIp(uint32_t ip);

/**
 * @brief Converts a subnet prefix length to a bitmask.
 *
 * This function takes a subnet prefix length and converts it into
 * a bitmask in unsigned integer format.
 *
 * @param[in] prefix The subnet prefix length.
 * @return The corresponding bitmask as an unsigned integer. Returns 0 if the prefix is invalid or 0.
 */
uint32_t celix_utils_ipPrefixLengthToBitmask(int prefix);

/**
 * @brief Converts a netmask string to a prefix length.
 *
 * This function converts a netmask in string format (e.g., "255.255.255.0")
 * to its corresponding prefix length.
 *
 * If the conversation failed, an error message is logged to celix_err.
 *
 * @param[in] netmask The netmask in string format.
 * @return The prefix length or -1 if the conversion fails.
 */
int celix_utils_ipNetmaskToPrefixLength(const char* netmask);

/**
 * @brief Finds an IP address within a given subnet on the host's network interfaces.
 *
 * This function searches through the network interfaces of the host to find
 * an IP address that falls within the specified subnet. It analyzes the IP addresses
 * assigned to each network interface and checks if any of them match the given subnet criteria.
 * The function returns the first IP address that is within the specified subnet range.
 *
 * The input parameter is expected to be in CIDR notation.
 * CIDR notation is a concise format for specifying IP addresses ranges using IP address and subnet prefix length.
 * It takes the form of 'IP_ADDRESS/PREFIX_LENGTH' (e.g., "192.168.0.1/24").
 *
 * If an error occurred, an error message is logged to celix_err.
 *
 * @param[in] subnetCidrNotation The IP address with subnet prefix.
 * @param[out] foundIp A string containing an IP address within the specified subnet that is also
 * assigned to a network interface on the host, or NULL if no matching IP address is found. The caller is owner of the
 * returned string.
 * @return CELIX_SUCCESS if the IP address was found, CELIX_ILLEGAL_ARGUMENT if the subnet is invalid, CELIX_ENOMEM if
 * an error occurred or a errno value set by getifaddrs.
 */
celix_status_t celix_utils_findIpInSubnet(const char* subnetCidrNotation, char** foundIp);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_IP_UTILS_H */
