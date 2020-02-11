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

#ifndef ETCDLIB_GLOBAL_H_
#define ETCDLIB_GLOBAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "etcdlib.h"

#ifdef CELIX_ADD_DEPRECATED_ATTRIBUTES
#define DEP_ATTRIBUTE __attribute__((deprecated("etcd_ call are placed by etcdlib_ calls, use etcdlib.h instead of etcd.h")))
#else
#define DEP_ATTRIBUTE
#endif

/**
 * @desc Initialize the ETCD-LIB  with the server/port where Etcd can be reached.
 * @param const char* server. String containing the IP-number of the server.
 * @param int port. Port number of the server.
 * @param int flags. bitwise flags to control etcdlib initialization. 
 * @return 0 on success, non zero otherwise.
 */
int etcd_init(const char* server, int port, int flags) DEP_ATTRIBUTE;

/**
 * @desc Retrieve a single value from Etcd.
 * @param const char* key. The Etcd-key (Note: a leading '/' should be avoided)
 * @param char** value. The allocated memory contains the Etcd-value. The caller is responsible for freeing this memory.
 * @param int* modifiedIndex. If not NULL the Etcd-index of the last modified value.
 * @return 0 on success, non zero otherwise
 */
int etcd_get(const char* key, char** value, int* modifiedIndex) DEP_ATTRIBUTE;

/**
 * @desc Retrieve the contents of a directory. For every found key/value pair the given callback function is called.
 * @param const char* directory. The Etcd-directory which has to be searched for keys
 * @param etcd_key_value_callback callback. Callback function which is called for every found key
 * @param void *arg. Argument is passed to the callback function
 * @param int* modifiedIndex. If not NULL the Etcd-index of the last modified value.
 * @return 0 on success, non zero otherwise
 */
int etcd_get_directory(const char* directory, etcdlib_key_value_callback callback, void *arg, long long* modifiedIndex) DEP_ATTRIBUTE;

/**
 * @desc Setting an Etcd-key/value
 * @param const char* key. The Etcd-key (Note: a leading '/' should be avoided)
 * @param const char* value. The Etcd-value 
 * @param int ttl. If non-zero this is used as the TTL value
 * @param bool prevExist. If true the value is only set when the key already exists, if false it is always set
 * @return 0 on success, non zero otherwise
 */
int etcd_set(const char* key, const char* value, int ttl, bool prevExist) DEP_ATTRIBUTE;

/**
 * @desc Refresh the ttl of an existing key.
 * @param key the etcd key to refresh.
 * @param ttl the ttl value to use.
 * @return 0 on success, non zero otherwise.
 */
int etcd_refresh(const char *key, int ttl) DEP_ATTRIBUTE;

/**
 * @desc Setting an Etcd-key/value and checks if there is a different previous value
 * @param const char* key. The Etcd-key (Note: a leading '/' should be avoided)
 * @param const char* value. The Etcd-value 
 * @param int ttl. If non-zero this is used as the TTL value
 * @param bool always_write. If true the value is written, if false only when the given value is equal to the value in etcd.
 * @return 0 on success, non zero otherwise
 */
int etcd_set_with_check(const char* key, const char* value, int ttl, bool always_write) DEP_ATTRIBUTE;

/**
 * @desc Deleting an Etcd-key
 * @param const char* key. The Etcd-key (Note: a leading '/' should be avoided)
 * @return 0 on success, non zero otherwise
 */
int etcd_del(const char* key) DEP_ATTRIBUTE;

/**
 * @desc Watching an etcd directory for changes
 * @param const char* key. The Etcd-key (Note: a leading '/' should be avoided)
 * @param long long index. The Etcd-index which the watch has to be started on.
 * @param char** action. If not NULL, memory is allocated and contains the action-string. The caller is responsible of freeing the memory.
 * @param char** prevValue. If not NULL, memory is allocated and contains the previous value. The caller is responsible of freeing the memory.
 * @param char** value. If not NULL, memory is allocated and contains the new value. The caller is responsible of freeing the memory.
 * @param char** rkey. If not NULL, memory is allocated and contains the updated key. The caller is responsible of freeing the memory.
 * @param long long* modifiedIndex. If not NULL, the index of the modification is written.
 * @return ETCDLIB_RC_OK (0) on success, non zero otherwise. Note that a timeout is signified by a ETCDLIB_RC_TIMEOUT return code.
 */
int etcd_watch(const char* key, long long index, char** action, char** prevValue, char** value, char** rkey, long long* modifiedIndex) DEP_ATTRIBUTE;

#ifdef __cplusplus
}
#endif

#endif /*ETCDLIB_GLOBAL_H_ */
