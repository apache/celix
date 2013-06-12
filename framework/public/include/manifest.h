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
 * manifest.h
 *
 *  \date       Jul 5, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef MANIFEST_H_
#define MANIFEST_H_

#include <apr_general.h>

#include "properties.h"
#include "celix_errno.h"

struct manifest {
	apr_pool_t *pool;
	properties_pt mainAttributes;
	hash_map_pt attributes;
};

typedef struct manifest * manifest_pt;

celix_status_t manifest_create(apr_pool_t *pool, manifest_pt *manifest);
celix_status_t manifest_createFromFile(apr_pool_t *pool, char *filename, manifest_pt *manifest);

void manifest_clear(manifest_pt manifest);
properties_pt manifest_getMainAttributes(manifest_pt manifest);
celix_status_t manifest_getEntries(manifest_pt manifest, hash_map_pt *map);

celix_status_t manifest_read(manifest_pt manifest, char *filename);
void manifest_write(manifest_pt manifest, char * filename);

char * manifest_getValue(manifest_pt manifest, const char * name);

#endif /* MANIFEST_H_ */
