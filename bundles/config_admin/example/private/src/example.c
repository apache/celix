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

#include "example.h"

#include <stdlib.h>
#include <pthread.h>

#include "hash_map.h"

struct example {
};

//Create function
celix_status_t example_create(example_pt *result) {
	celix_status_t status = CELIX_SUCCESS;
	
    example_pt component = calloc(1, sizeof(*component));
	if (component != NULL) {
		(*result) = component;
	} else {
		status = CELIX_ENOMEM;
	}	
	return status;
}

celix_status_t example_destroy(example_pt component) {
	celix_status_t status = CELIX_SUCCESS;
	if (component != NULL) {
		free(component);
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}
	return status;
}

celix_status_t example_start(example_pt component) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t example_stop(example_pt component) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t example_updated(example_pt component, properties_pt updatedProperties) {
    printf("updated called\n");
    hash_map_pt map = (hash_map_pt)updatedProperties;
    if (map != NULL) {
        hash_map_iterator_pt iter = hashMapIterator_create(map);
        while(hashMapIterator_hasNext(iter)) {
            char *key = hashMapIterator_nextKey(iter);
            const char *value = properties_get(updatedProperties, key);
            printf("got property %s:%s\n", key, value);
        }
    } else {
        printf("updated with NULL properties\n");
    }
    return CELIX_SUCCESS;
}
