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
 * driver_matcher.c
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include "hash_map.h"
#include "celix_constants.h"

#include "driver_matcher.h"
#include "celix_log_helper.h"


struct driver_matcher {
	hash_map_pt attributes;
	array_list_pt matches;
    celix_log_helper_t *loghelper;

	bundle_context_pt context;
};

typedef struct match_key {
	int matchValue;
}*match_key_t;

static celix_status_t driverMatcher_get(driver_matcher_pt matcher, int key, array_list_pt *attributesV);
static celix_status_t driverMatcher_getBestMatchInternal(driver_matcher_pt matcher, match_pt *match);

unsigned int driverMatcher_matchKeyHash(const void* match_key) {
	match_key_t key = (match_key_t) match_key;

	return key->matchValue;
}

int driverMatcher_matchKeyEquals(const void* key, const void* toCompare) {
	return ((match_key_t) key)->matchValue == ((match_key_t) toCompare)->matchValue;
}

celix_status_t driverMatcher_create(bundle_context_pt context, driver_matcher_pt *matcher) {
	celix_status_t status = CELIX_SUCCESS;

	*matcher = calloc(1, sizeof(**matcher));
	if (!*matcher) {
		status = CELIX_ENOMEM;
	} else {
		(*matcher)->matches = NULL;
		(*matcher)->context = context;
		(*matcher)->attributes = hashMap_create(driverMatcher_matchKeyHash, NULL, driverMatcher_matchKeyEquals, NULL);

		arrayList_create(&(*matcher)->matches);

        (*matcher)->loghelper = celix_logHelper_create(context, "celix_device_access");
	}

	return status;
}

celix_status_t driverMatcher_destroy(driver_matcher_pt *matcher) {

	if((*matcher) != NULL){

		int i = 0;

		for(;i<arrayList_size((*matcher)->matches);i++){
			free(arrayList_get((*matcher)->matches,i));
		}
		arrayList_destroy((*matcher)->matches);

		hash_map_iterator_pt iter = hashMapIterator_create((*matcher)->attributes);
		while (hashMapIterator_hasNext(iter)) {
			hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
			match_key_t match = (match_key_t)hashMapEntry_getKey(entry);
			array_list_pt list = (array_list_pt)hashMapEntry_getValue(entry);
			free(match);
			if (list != NULL) {
				arrayList_destroy(list);
			}
		}
		hashMapIterator_destroy(iter);
		hashMap_destroy((*matcher)->attributes, false, false);

		celix_logHelper_destroy((*matcher)->loghelper);

		free(*matcher);
	}

	return CELIX_SUCCESS;
}

celix_status_t driverMatcher_add(driver_matcher_pt matcher, int matchValue, driver_attributes_pt attributes) {
	celix_status_t status = CELIX_SUCCESS;

	array_list_pt da = NULL;
	status = driverMatcher_get(matcher, matchValue, &da);
	if (status == CELIX_SUCCESS) {
		arrayList_add(da, attributes);

		match_pt match = NULL;
		match = calloc(1, sizeof(*match));
		if (!match) {
			status = CELIX_ENOMEM;
		} else {
			match->matchValue = matchValue;
			match->reference = NULL;
			driverAttributes_getReference(attributes, &match->reference);
			arrayList_add(matcher->matches, match);
		}
	}

	return status;
}

celix_status_t driverMatcher_get(driver_matcher_pt matcher, int key, array_list_pt *attributes) {
	celix_status_t status = CELIX_SUCCESS;

	match_key_t matchKeyS = calloc(1, sizeof(*matchKeyS));
	matchKeyS->matchValue = key;

	*attributes = hashMap_get(matcher->attributes, matchKeyS);
	if (*attributes == NULL) {
		arrayList_create(attributes);
		match_key_t matchKey = calloc(1, sizeof(*matchKey));
		matchKey->matchValue = key;
		hashMap_put(matcher->attributes, matchKey, *attributes);
	}

	free(matchKeyS);

	return status;
}

celix_status_t driverMatcher_getBestMatch(driver_matcher_pt matcher, service_reference_pt reference, match_pt *match) {
	celix_status_t status = CELIX_SUCCESS;

	if (*match != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		service_reference_pt selectorRef = NULL;
		status = bundleContext_getServiceReference(matcher->context, OSGI_DEVICEACCESS_DRIVER_SELECTOR_SERVICE_NAME, &selectorRef);
		if (status == CELIX_SUCCESS) {
			int index = -1;
			if (selectorRef != NULL) {
				driver_selector_service_pt selector = NULL;
				status = bundleContext_getService(matcher->context, selectorRef, (void **) &selector);
				if (status == CELIX_SUCCESS) {
					if (selector != NULL) {
						int size = -1;
						status = selector->driverSelector_select(selector->selector, reference, matcher->matches, &index);
						if (status == CELIX_SUCCESS) {
							size = arrayList_size(matcher->matches);
							if (index != -1 && index >= 0 && index < size) {
								*match = arrayList_get(matcher->matches, index);
							}
						}
					}
				}
			}
			if (status == CELIX_SUCCESS && *match == NULL) {
				status = driverMatcher_getBestMatchInternal(matcher, match);
			}
		}
	}

	return status;
}

celix_status_t driverMatcher_getBestMatchInternal(driver_matcher_pt matcher, match_pt *match) {
	celix_status_t status = CELIX_SUCCESS;

	if (!hashMap_isEmpty(matcher->attributes)) {
		match_key_t matchKey = NULL;
		hash_map_iterator_pt iter = hashMapIterator_create(matcher->attributes);
		while (hashMapIterator_hasNext(iter)) {
			hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
			match_key_t key = hashMapEntry_getKey(entry);
			if (matchKey == NULL || matchKey->matchValue < key->matchValue) {
				matchKey = key;
			}
		}
		hashMapIterator_destroy(iter);

		array_list_pt das = hashMap_get(matcher->attributes, matchKey);
		service_reference_pt best = NULL;
		int i;
		for (i = 0; i < arrayList_size(das); i++) {
			driver_attributes_pt attributes = arrayList_get(das, i);
			service_reference_pt reference = NULL;

			celix_status_t substatus = driverAttributes_getReference(attributes, &reference);
			if (substatus == CELIX_SUCCESS) {
				if (best != NULL) {
					const char* rank1Str;
					const char* rank2Str;
					int rank1, rank2;

					rank1Str = "0";
					rank2Str = "0";

                    celix_logHelper_log(matcher->loghelper, CELIX_LOG_LEVEL_DEBUG, "DRIVER_MATCHER: Compare ranking");

					serviceReference_getProperty(reference, OSGI_FRAMEWORK_SERVICE_RANKING, &rank1Str);
					serviceReference_getProperty(reference, OSGI_FRAMEWORK_SERVICE_RANKING, &rank2Str);

					rank1 = atoi(rank1Str);
					rank2 = atoi(rank2Str);

					if (rank1 != rank2) {
						if (rank1 > rank2) {
							best = reference;
						}
					} else {
						const char* id1Str;
						const char* id2Str;
						long id1, id2;

						id1Str = NULL;
						id2Str = NULL;

                        celix_logHelper_log(matcher->loghelper, CELIX_LOG_LEVEL_DEBUG, "DRIVER_MATCHER: Compare id's");

						serviceReference_getProperty(reference, OSGI_FRAMEWORK_SERVICE_ID, &id1Str);
						serviceReference_getProperty(reference, OSGI_FRAMEWORK_SERVICE_ID, &id2Str);

						id1 = atol(id1Str);
						id2 = atol(id2Str);

						if (id1 < id2) {
							best = reference;
						}
					}
				} else {
					best = reference;
				}
			}

		}

		*match = calloc(1, sizeof(**match));
		if (!*match) {
			status = CELIX_ENOMEM;
		} else {
			(*match)->matchValue = matchKey->matchValue;
			(*match)->reference = best;
		}
	}

	return status;
}
