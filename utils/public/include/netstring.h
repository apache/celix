/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * netstring.h
 *
 *  \date       Sep 13, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef NETSTRING_H_
#define NETSTRING_H_

#include <apr_general.h>
#include "array_list.h"
#include "hash_map.h"
#include "exports.h"
#include "celix_errno.h"


typedef celix_status_t (*customEncoderCallback) (apr_pool_t *pool, void* inPtr, char **outNetStr);
typedef celix_status_t (*customDecoderCallback) (apr_pool_t *pool, char* inNetStr, void *outPtr);

UTILS_EXPORT celix_status_t netstring_encode(apr_pool_t *pool, char* inStr, char **outNetStr);
UTILS_EXPORT celix_status_t netstring_decode(apr_pool_t *pool, char* inNetStr, char **outStr);
UTILS_EXPORT celix_status_t netstring_decodetoArray(apr_pool_t *pool, char* inNetStr, char **outStrArr[], int *outArrSize);
UTILS_EXPORT celix_status_t netstring_encodeFromArray(apr_pool_t *pool, char *inStrArr[], int inArrLen, char** outNetStr);
UTILS_EXPORT celix_status_t netstring_custDecodeFromArray(apr_pool_t *pool, char* inNetStr, customDecoderCallback inCallback, void **outStrArr[], int *outArrSize);
UTILS_EXPORT celix_status_t netstring_custEncodeFromArray(apr_pool_t *pool, void *inStrArr[], customEncoderCallback inCallback, int inArrLen, char** outNetStr);
UTILS_EXPORT celix_status_t netstring_decodeToHashMap(apr_pool_t *pool, char* inNetStr, hash_map_pt outHashMap);
UTILS_EXPORT celix_status_t netstring_encodeFromHashMap(apr_pool_t *pool, hash_map_pt inHashMap, char** outNetStr);
UTILS_EXPORT celix_status_t netstring_custDecodeToHashMap(apr_pool_t *pool, char* inNetStr, customDecoderCallback keyDecCallback, customDecoderCallback valueDecCallback, hash_map_pt outHashMap);
UTILS_EXPORT celix_status_t netstring_custEncodeFromHashMap(apr_pool_t *pool, hash_map_pt inHashMap, customEncoderCallback keyEncCallback, customEncoderCallback valueEncCallback, char** outNetStr);
UTILS_EXPORT celix_status_t netstring_encodeFromArrayList(apr_pool_t *pool, array_list_pt inList, char** outNetStr);
UTILS_EXPORT celix_status_t netstring_decodeToArrayList(apr_pool_t *pool, char* inNetStr, array_list_pt outArrayList);
UTILS_EXPORT celix_status_t netstring_custEncodeFromArrayList(apr_pool_t *pool, array_list_pt inList, customEncoderCallback elCallback, char** outNetStr);
UTILS_EXPORT celix_status_t netstring_custDecodeToArrayList(apr_pool_t *pool, char* inNetStr, customDecoderCallback decCallback, array_list_pt outArrayList);



#endif /* NETSTRING_H_ */
