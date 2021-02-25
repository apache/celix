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
#include <stdio.h>
#include <string.h>
#include "pubsub_tcp_common.h"


bool psa_tcp_isPassive(const char* buffer) {
    bool isPassive = false;
    // Parse Properties
    if (buffer != NULL) {
        char buf[32];
        snprintf(buf, 32, "%s", buffer);
        char *trimmed = utils_stringTrim(buf);
        if (strncasecmp("true", trimmed, strlen("true")) == 0) {
            isPassive = true;
        } else if (strncasecmp("false", trimmed, strlen("false")) == 0) {
            isPassive = false;
        }
    }
    return isPassive;
}
