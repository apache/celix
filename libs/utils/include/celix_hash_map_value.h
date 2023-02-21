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

#ifndef CELIX_HASH_MAP_VALUE_H_
#define CELIX_HASH_MAP_VALUE_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Represents a value in the hash map.
 *
 * Because the value of hash map entry can be a pointer, long, double or boolean, the value is represented as an union.
 */
typedef union celix_hash_map_value {
    void* ptrValue;
    long longValue;
    double doubleValue;
    bool boolValue;
} celix_hash_map_value_t;


#ifdef __cplusplus
}
#endif

#endif //CELIX_HASH_MAP_VALUE_H_
