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
 * ip_utils_test.cpp
 *
 *  \date       Jun 24, 2019
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "string.h"
#include <stdlib.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C"
{
#include "ip_utils.h"
}

int main(int argc, char** argv) {
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    return RUN_ALL_TESTS(argc, argv);
}

static char* my_strdup(const char* s){
    if(s==NULL){
        return NULL;
    }

    size_t len = strlen(s);

    char *d = (char*) calloc (len + 1,sizeof(char));

    if (d == NULL){
        return NULL;
    }

    strncpy (d,s,len);
    return d;
}

TEST_GROUP(ip_utils) {
    void setup(void) {
    }

    void teardown() {
    }
};

TEST(ip_utils, ipToUnsignedInt){
    char *ip = my_strdup("192.168.1.64");

    unsigned int expected = 3232235840;
    unsigned int actual = ipUtils_ipToUnsignedInt(ip);
    UNSIGNED_LONGS_EQUAL(expected, actual);

    free(ip);
}

TEST(ip_utils, unsignedIntToIp){
    unsigned int ipAsUint = 3232235840;

    const char *expected = "192.168.1.64";
    char *actual = ipUtils_unsignedIntToIp(ipAsUint);
    STRCMP_EQUAL(expected, actual);
    free(actual);
}

TEST(ip_utils, prefixToBitmask){
    unsigned int expected = 4294967264;
    unsigned int actual = ipUtils_prefixToBitmask(27);

    UNSIGNED_LONGS_EQUAL(expected, actual);
}

TEST(ip_utils, netmaskToPrefix){
    char *netmask = my_strdup("255.255.255.0");

    int expected = 24;
    int actual = ipUtils_netmaskToPrefix(netmask);
    LONGS_EQUAL(expected, actual);

    free(netmask);
}
