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
 * @file celix_properties_private.h
 * @brief Private Header file for the Celix Properties used for whitebox testing.
 */

#ifndef CELIX_CELIX_PROPERTIES_PRIVATE_H
#define CELIX_CELIX_PROPERTIES_PRIVATE_H

#include "celix_properties.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Alloc new entry for the provided properties. Possible using the properties optimizer cache.
 */
celix_properties_entry_t* celix_properties_allocEntry(celix_properties_t* properties);

/**
 * @brief Create a new string for the provided properties. Possible using the properties optimizer cache.
 */
char* celix_properties_createString(celix_properties_t* properties, const char* str);


#ifdef __cplusplus
}
#endif

#endif // CELIX_CELIX_PROPERTIES_PRIVATE_H