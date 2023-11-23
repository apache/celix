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
#include "celix_long_hash_map_ei.h"

extern "C" {

celix_long_hash_map_t* __real_celix_longHashMap_create(void);
CELIX_EI_DEFINE(celix_longHashMap_create, celix_long_hash_map_t*)
celix_long_hash_map_t* __wrap_celix_longHashMap_create(void) {
    CELIX_EI_IMPL(celix_longHashMap_create);
    return __real_celix_longHashMap_create();
}

celix_long_hash_map_t* __real_celix_longHashMap_createWithOptions(const celix_long_hash_map_create_options_t* opts);
CELIX_EI_DEFINE(celix_longHashMap_createWithOptions, celix_long_hash_map_t*);
celix_long_hash_map_t* __wrap_celix_longHashMap_createWithOptions(const celix_long_hash_map_create_options_t* opts) {
    CELIX_EI_IMPL(celix_longHashMap_createWithOptions);
    return __real_celix_longHashMap_createWithOptions(opts);
}

celix_status_t __real_celix_longHashMap_put(celix_long_hash_map_t* map, long key, void* value);
CELIX_EI_DEFINE(celix_longHashMap_put, celix_status_t)
celix_status_t __wrap_celix_longHashMap_put(celix_long_hash_map_t* map, long key, void* value) {
    CELIX_EI_IMPL(celix_longHashMap_put);
    return __real_celix_longHashMap_put(map, key, value);
}

celix_status_t __real_celix_longHashMap_putLong(celix_long_hash_map_t* map, long key, long value);
CELIX_EI_DEFINE(celix_longHashMap_putLong, celix_status_t)
celix_status_t __wrap_celix_longHashMap_putLong(celix_long_hash_map_t* map, long key, long value) {
    CELIX_EI_IMPL(celix_longHashMap_putLong);
    return __real_celix_longHashMap_putLong(map, key, value);
}

celix_status_t __real_celix_longHashMap_putDouble(celix_long_hash_map_t* map, long key, double value);
CELIX_EI_DEFINE(celix_longHashMap_putDouble, celix_status_t)
celix_status_t __wrap_celix_longHashMap_putDouble(celix_long_hash_map_t* map, long key, double value) {
    CELIX_EI_IMPL(celix_longHashMap_putDouble);
    return __real_celix_longHashMap_putDouble(map, key, value);
}

celix_status_t __real_celix_longHashMap_putBool(celix_long_hash_map_t* map, long key, bool value);
CELIX_EI_DEFINE(celix_longHashMap_putBool, celix_status_t)
celix_status_t __wrap_celix_longHashMap_putBool(celix_long_hash_map_t* map, long key, bool value) {
    CELIX_EI_IMPL(celix_longHashMap_putBool);
    return __real_celix_longHashMap_putBool(map, key, value);
}

}