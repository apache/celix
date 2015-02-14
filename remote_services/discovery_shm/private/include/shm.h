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
 * shm.h
 *
 *  \date       26 Jul 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */



#ifndef _SHM_H_
#define _SHM_H_

#include <celix_errno.h>

#define SHM_ENTRY_MAX_KEY_LENGTH	256
#define SHM_ENTRY_MAX_VALUE_LENGTH	256

// defines the time-to-live in seconds
#define SHM_ENTRY_DEFAULT_TTL		60

// we currently support 64 separate discovery instances
#define SHM_DATA_MAX_ENTRIES		64

typedef struct shmData* shmData_pt;

/* creates a new shared memory block */
celix_status_t discovery_shmCreate(shmData_pt* data);
celix_status_t discovery_shmAttach(shmData_pt* data);
celix_status_t discovery_shmSet(shmData_pt data, char *key, char* value);
celix_status_t discovery_shmGet(shmData_pt data, char* key, char* value);
celix_status_t discovery_shmGetKeys(shmData_pt data, char** keys, int* size);
celix_status_t discovery_shmRemove(shmData_pt data, char* key);
celix_status_t discovery_shmDetach(shmData_pt data);
celix_status_t discovery_shmDestroy(shmData_pt data);

#endif
