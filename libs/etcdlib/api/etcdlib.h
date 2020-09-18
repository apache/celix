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

#ifndef ETCDLIB_H_
#define ETCDLIB_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

/*
 * If set etcdlib will _not_ initialize curl
 * using curl_global_init. Note that 
 * curl_global_init can be called multiple
 * times, but is _not_ thread-safe.
 */
#define ETCDLIB_NO_CURL_INITIALIZATION (1)

#define ETCDLIB_ACTION_CREATE   "create"
#define ETCDLIB_ACTION_GET      "get"
#define ETCDLIB_ACTION_SET      "set"
#define ETCDLIB_ACTION_UPDATE   "update"
#define ETCDLIB_ACTION_DELETE   "delete"
#define ETCDLIB_ACTION_EXPIRE   "expire"

#define ETCDLIB_RC_OK           0
#define ETCDLIB_RC_ERROR        1
#define ETCDLIB_RC_TIMEOUT      2

typedef struct etcdlib_struct etcdlib_t; //opaque struct

typedef void (*etcdlib_key_value_callback) (const char *key, const char *value, void* arg);

/**
 * @desc Creates the ETCD-LIB  with the server/port where Etcd can be reached.
 * @param const char* server. String containing the IP-number of the server.
 * @param int port. Port number of the server.
 * @param int flags. bitwise flags to control etcdlib initialization. 
 * @return Pointer to the etcdlib_t struct needed by subsequent api calls
 */
etcdlib_t* etcdlib_create(const char* server, int port, int flags);

/**
 * @desc Destroys the ETCD-LIB.  with the server/port where Etcd can be reached.
 * @param etcdlib_t* The ETCD-LIB instance.
 */
void etcdlib_destroy(etcdlib_t *etcdlib);

/**
 * Returns the configured etcd host for etcdlib.
 */
const char* etcdlib_host(etcdlib_t *etcdlib);

/**
 * Returns the configured etcd port for etcdlib.
 */
int etcdlib_port(etcdlib_t *etcdlib);

/**
 * @desc Retrieve a single value from Etcd.
 * @param const etcdlib_t* etcdlib. The ETCD-LIB instance (contains hostname and port info).
 * @param const char* key. The Etcd-key (Note: a leading '/' should be avoided).
 * @param char** value. The allocated memory contains the Etcd-value. The caller is responsible for freeing this memory.
 * @param int* modifiedIndex. If not NULL the Etcd-index of the last modified value.
 * @return 0 on success, non zero otherwise
 */
int etcdlib_get(etcdlib_t *etcdlib, const char* key, char** value, int* modifiedIndex);

/**
 * @desc Retrieve the contents of a directory. For every found key/value pair the given callback function is called.
 * @param const etcdlib_t* etcdlib. The ETCD-LIB instance (contains hostname and port info).
 * @param const char* directory. The Etcd-directory which has to be searched for keys
 * @param etcdlib_key_value_callback callback. Callback function which is called for every found key
 * @param void *arg. Argument is passed to the callback function
 * @param int* modifiedIndex. If not NULL the Etcd-index of the last modified value.
 * @return 0 on success, non zero otherwise
 */
int etcdlib_get_directory(etcdlib_t *etcdlib, const char* directory, etcdlib_key_value_callback callback, void *arg, long long* modifiedIndex);

/**
 * @desc Setting an Etcd-key/value
 * @param const etcdlib_t* etcdlib. The ETCD-LIB instance (contains hostname and port info).
 * @param const char* key. The Etcd-key (Note: a leading '/' should be avoided)
 * @param const char* value. The Etcd-value 
 * @param int ttl. If non-zero this is used as the TTL value
 * @param bool prevExist. If true the value is only set when the key already exists, if false it is always set
 * @return 0 on success, non zero otherwise
 */
int etcdlib_set(etcdlib_t *etcdlib, const char* key, const char* value, int ttl, bool prevExist);

/**
 * @desc Refresh the ttl of an existing key.
 * @param const etcdlib_t* etcdlib. The ETCD-LIB instance (contains hostname and port info).
 * @param key the etcd key to refresh.
 * @param ttl the ttl value to use.
 * @return 0 on success, non zero otherwise.
 */
int etcdlib_refresh(etcdlib_t *etcdlib, const char *key, int ttl);

/**
 * @desc Setting an Etcd-key/value and checks if there is a different previous value
 * @param const etcdlib_t* etcdlib. The ETCD-LIB instance (contains hostname and port info).
 * @param const char* key. The Etcd-key (Note: a leading '/' should be avoided)
 * @param const char* value. The Etcd-value 
 * @param int ttl. If non-zero this is used as the TTL value
 * @param bool always_write. If true the value is written, if false only when the given value is equal to the value in etcd.
 * @return 0 on success, non zero otherwise
 */
int etcdlib_set_with_check(etcdlib_t *etcdlib, const char* key, const char* value, int ttl, bool always_write);

/**
 * @desc Deleting an Etcd-key
 * @param const etcdlib_t* etcdlib. The ETCD-LIB instance (contains hostname and port info).
 * @param const char* key. The Etcd-key (Note: a leading '/' should be avoided)
 * @return 0 on success, non zero otherwise
 */
int etcdlib_del(etcdlib_t *etcdlib, const char* key);

/**
 * @desc Watching an etcd directory for changes
 * @param const etcdlib_t* etcdlib. The ETCD-LIB instance (contains hostname and port info).
 * @param const char* key. The Etcd-key (Note: a leading '/' should be avoided)
 * @param long long index. The Etcd-index which the watch has to be started on.
 * @param char** action. If not NULL, memory is allocated and contains the action-string. The caller is responsible of freeing the memory.
 * @param char** prevValue. If not NULL, memory is allocated and contains the previous value. The caller is responsible of freeing the memory.
 * @param char** value. If not NULL, memory is allocated and contains the new value. The caller is responsible of freeing the memory.
 * @param char** rkey. If not NULL, memory is allocated and contains the updated key. The caller is responsible of freeing the memory.
 * @param long long* modifiedIndex. If not NULL, the index of the modification is written.
 * @return ETCDLIB_RC_OK (0) on success, non zero otherwise. Note that a timeout is signified by a ETCDLIB_RC_TIMEOUT return code.
 */
int etcdlib_watch(etcdlib_t *etcdlib, const char* key, long long index, char** action, char** prevValue, char** value, char** rkey, long long* modifiedIndex);

#ifdef __cplusplus
}
#endif

#endif /*ETCDLIB_H_ */
