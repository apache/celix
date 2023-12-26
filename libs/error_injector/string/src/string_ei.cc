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
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#include "string_ei.h"
#include <errno.h>

extern "C" {
char *__real_strndup(const char *s, size_t n);
CELIX_EI_DEFINE(strndup, char*)
char *__wrap_strndup(const char *s, size_t n) {
    errno = ENOMEM;
    CELIX_EI_IMPL(strndup);
    errno = 0;
    return __real_strndup(s, n);
}

char *__real_strdup(const char *s);
CELIX_EI_DEFINE(strdup, char*)
char *__wrap_strdup(const char *s) {
    errno = ENOMEM;
    CELIX_EI_IMPL(strdup);
    errno = 0;
    return __real_strdup(s);
}

}