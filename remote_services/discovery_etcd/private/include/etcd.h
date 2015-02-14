/**
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

/*
 * etcd.h
 *
 *  \date       26 Jul 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#ifndef ETCD_H_
#define ETCD_H_

#include <stdbool.h>
#include <celix_errno.h>

#define MAX_NODES			256

#define MAX_KEY_LENGTH		256
#define MAX_VALUE_LENGTH	256
#define MAX_ACTION_LENGTH	64

#define MAX_URL_LENGTH		256
#define MAX_CONTENT_LENGTH	1024

#define ETCD_JSON_NODE			"node"
#define ETCD_JSON_PREVNODE		"prevNode"
#define ETCD_JSON_NODES			"nodes"
#define ETCD_JSON_ACTION		"action"
#define ETCD_JSON_KEY			"key"
#define ETCD_JSON_VALUE			"value"
#define ETCD_JSON_MODIFIEDINDEX	"modifiedIndex"

celix_status_t etcd_init(char* server, int port);
bool etcd_get(char* key, char* value, char*action, int* modifiedIndex);
bool etcd_getNodes(char* directory, char** nodeNames, int* size);
bool etcd_set(char* key, char* value, int ttl, bool prevExist);
bool etcd_del(char* key);
bool etcd_watch(char* key, int index, char* action, char* prevValue, char* value);

#endif /* ETCD_H_ */
