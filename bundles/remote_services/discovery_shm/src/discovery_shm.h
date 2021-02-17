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
 * shm.h
 *
 *  \date       26 Jul 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef _DISCOVERY_SHM_H_
#define _DISCOVERY_SHM_H_

#include <celix_errno.h>

#define SHM_ENTRY_MAX_KEY_LENGTH	256
#define SHM_ENTRY_MAX_VALUE_LENGTH	256

// defines the time-to-live in seconds
#define SHM_ENTRY_DEFAULT_TTL		60

// we currently support 64 separate discovery instances
#define SHM_DATA_MAX_ENTRIES		64

typedef struct shmData shmData_t;

/* creates a new shared memory block */
celix_status_t discoveryShm_create(shmData_t **data);
celix_status_t discoveryShm_attach(shmData_t **data);
celix_status_t discoveryShm_set(shmData_t *data, char *key, char* value);
celix_status_t discoveryShm_get(shmData_t *data, char* key, char* value);
celix_status_t discoveryShm_getKeys(shmData_t *data, char** keys, int* size);
celix_status_t discoveryShm_remove(shmData_t *data, char* key);
celix_status_t discoveryShm_detach(shmData_t *data);
celix_status_t discoveryShm_destroy(shmData_t *data);

#endif
