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
#include "mosquitto_ei.h"

extern "C" {

struct mosquitto* __real_mosquitto_new(const char* id, bool clean_session, void* obj);
CELIX_EI_DEFINE(mosquitto_new, struct mosquitto*)
struct mosquitto* __wrap_mosquitto_new(const char* id, bool clean_session, void* obj) {
    CELIX_EI_IMPL(mosquitto_new);
    return __real_mosquitto_new(id, clean_session, obj);
}

int __real_mosquitto_property_add_int32(struct mosquitto* mosq, int identifier, int value);
CELIX_EI_DEFINE(mosquitto_property_add_int32, int)
int __wrap_mosquitto_property_add_int32(struct mosquitto* mosq, int identifier, int value) {
    CELIX_EI_IMPL(mosquitto_property_add_int32);
    return __real_mosquitto_property_add_int32(mosq, identifier, value);
}

int __real_mosquitto_property_add_string_pair(struct mosquitto* mosq, int identifier, const char* key, const char* value);
CELIX_EI_DEFINE(mosquitto_property_add_string_pair, int)
int __wrap_mosquitto_property_add_string_pair(struct mosquitto* mosq, int identifier, const char* key, const char* value) {
    CELIX_EI_IMPL(mosquitto_property_add_string_pair);
    return __real_mosquitto_property_add_string_pair(mosq, identifier, key, value);
}

int __real_mosquitto_property_add_string(struct mosquitto* mosq, int identifier, const char* value);
CELIX_EI_DEFINE(mosquitto_property_add_string, int)
int __wrap_mosquitto_property_add_string(struct mosquitto* mosq, int identifier, const char* value) {
    CELIX_EI_IMPL(mosquitto_property_add_string);
    return __real_mosquitto_property_add_string(mosq, identifier, value);
}

int __real_mosquitto_property_add_binary(struct mosquitto* mosq, int identifier, const void* value, int len);
CELIX_EI_DEFINE(mosquitto_property_add_binary, int)
int __wrap_mosquitto_property_add_binary(struct mosquitto* mosq, int identifier, const void* value, int len) {
    CELIX_EI_IMPL(mosquitto_property_add_binary);
    return __real_mosquitto_property_add_binary(mosq, identifier, value, len);
}

int __real_mosquitto_int_option(struct mosquitto* mosq, int option, int value);
CELIX_EI_DEFINE(mosquitto_int_option, int)
int __wrap_mosquitto_int_option(struct mosquitto* mosq, int option, int value) {
    CELIX_EI_IMPL(mosquitto_int_option);
    return __real_mosquitto_int_option(mosq, option, value);
}

int __real_mosquitto_will_set_v5(struct mosquitto* mosq, const char* topic, int payloadlen, const void* payload, int qos, bool retain, const mosquitto_property* properties);
CELIX_EI_DEFINE(mosquitto_will_set_v5, int)
int __wrap_mosquitto_will_set_v5(struct mosquitto* mosq, const char* topic, int payloadlen, const void* payload, int qos, bool retain, const mosquitto_property* properties) {
    CELIX_EI_IMPL(mosquitto_will_set_v5);
    return __real_mosquitto_will_set_v5(mosq, topic, payloadlen, payload, qos, retain, properties);
}

int __real_mosquitto_subscribe_v5(struct mosquitto* mosq, int* mid, const char* sub, int qos, int options, const mosquitto_property* properties);
CELIX_EI_DEFINE(mosquitto_subscribe_v5, int)
int __wrap_mosquitto_subscribe_v5(struct mosquitto* mosq, int* mid, const char* sub, int qos, int options, const mosquitto_property* properties) {
    CELIX_EI_IMPL(mosquitto_subscribe_v5);
    return __real_mosquitto_subscribe_v5(mosq, mid, sub, qos, options, properties);
}

int __real_mosquitto_unsubscribe(struct mosquitto* mosq, int* mid, const char* sub);
CELIX_EI_DEFINE(mosquitto_unsubscribe, int)
int __wrap_mosquitto_unsubscribe(struct mosquitto* mosq, int* mid, const char* sub) {
    CELIX_EI_IMPL(mosquitto_unsubscribe);
    return __real_mosquitto_unsubscribe(mosq, mid, sub);
}

int __real_mosquitto_publish_v5(struct mosquitto* mosq, int* mid, const char* topic, int payloadlen, const void* payload, int qos, bool retain, const mosquitto_property* properties);
CELIX_EI_DEFINE(mosquitto_publish_v5, int)
int __wrap_mosquitto_publish_v5(struct mosquitto* mosq, int* mid, const char* topic, int payloadlen, const void* payload, int qos, bool retain, const mosquitto_property* properties) {
    CELIX_EI_IMPL(mosquitto_publish_v5);
    return __real_mosquitto_publish_v5(mosq, mid, topic, payloadlen, payload, qos, retain, properties);
}

int __real_mosquitto_property_copy_all(struct mosquitto* mosq, const mosquitto_property* src);
CELIX_EI_DEFINE(mosquitto_property_copy_all, int)
int __wrap_mosquitto_property_copy_all(struct mosquitto* mosq, const mosquitto_property* src) {
    CELIX_EI_IMPL(mosquitto_property_copy_all);
    return __real_mosquitto_property_copy_all(mosq, src);
}

const mosquitto_property* __real_mosquitto_property_read_string(const mosquitto_property* property, int identifier, const char** value, bool skip_first);
CELIX_EI_DEFINE(mosquitto_property_read_string, const mosquitto_property*)
const mosquitto_property* __wrap_mosquitto_property_read_string(const mosquitto_property* property, int identifier, const char** value, bool skip_first) {
    CELIX_EI_IMPL(mosquitto_property_read_string);
    return __real_mosquitto_property_read_string(property, identifier, value, skip_first);
}

const mosquitto_property* __real_mosquitto_property_read_binary(const mosquitto_property* property, int identifier, const void** value, int* len, bool skip_first);
CELIX_EI_DEFINE(mosquitto_property_read_binary, const mosquitto_property*)
const mosquitto_property* __wrap_mosquitto_property_read_binary(const mosquitto_property* property, int identifier, const void** value, int* len, bool skip_first) {
    CELIX_EI_IMPL(mosquitto_property_read_binary);
    return __real_mosquitto_property_read_binary(property, identifier, value, len, skip_first);
}

const mosquitto_property* __real_mosquitto_property_read_string_pair(const mosquitto_property* property, int identifier, const char** name, const char** value, bool skip_first);
CELIX_EI_DEFINE(mosquitto_property_read_string_pair, const mosquitto_property*)
const mosquitto_property* __wrap_mosquitto_property_read_string_pair(const mosquitto_property* property, int identifier, const char** name, const char** value, bool skip_first) {
    CELIX_EI_IMPL(mosquitto_property_read_string_pair);
    return __real_mosquitto_property_read_string_pair(property, identifier, name, value, skip_first);
}

} // extern "C"
