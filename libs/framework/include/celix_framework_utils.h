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

#ifndef CELIX_FRAMEWORK_UTILS_H_
#define CELIX_FRAMEWORK_UTILS_H_


#include "celix_framework.h"
#include "celix_array_list.h"

#ifdef __cplusplus
extern "C" {
#endif

//TODO make sep for embedded list a space and check if bundle file path with spaces are supported (escape char stuf).
//TODO doc
celix_array_list_t* celix_framework_utils_listEmbeddedBundles();

//TODO doc
size_t celix_framework_utils_installEmbeddedBundles(celix_framework_t* fw, bool autoStart);

//TODO make cmake command to add bundles as dep and create a compile definition with a bundle set
//TODO
//size_t celix_framework_utils_installBundlesSet(celix_framework_t* fw, const char* bundleSet, bool autoStart);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_FRAMEWORK_UTILS_H_ */
