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

#ifndef CELIX_UTILS_H_
#define CELIX_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CELIX_UTILS_MAX_STRLEN      1024*1024*10

/**
 * Creates a copy of a provided string.
 * The strdup is limited to the CELIX_UTILS_MAX_STRLEN and uses strndup to achieve this.
 * @return a copy of the string (including null terminator).
 */
char* celix_utils_strdup(const char *str);

#ifdef __cplusplus
}
#endif
#endif /* CELIX_UTILS_H_ */
