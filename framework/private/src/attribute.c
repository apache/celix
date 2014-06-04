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
 * attribute.c
 *
 *  \date       Jul 27, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "attribute_private.h"
#include "celix_log.h"

celix_status_t attribute_create(char * key, char * value, attribute_pt *attribute) {
	celix_status_t status = CELIX_SUCCESS;
	char *error = NULL;

	if (key == NULL || value == NULL || *attribute != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	    error = "Missing required arguments and/or values";
	} else {
		attribute_pt attr = malloc(sizeof(*attr));
		if (!attr) {
			status = CELIX_ENOMEM;
		} else {
			attr->key = key;
			attr->value = value;

			*attribute = attr;
		}
	}

	framework_logIfError(logger, status, error, "Could not create attribute: [key=%s;value=%s]", key, value);

	return status;
}

celix_status_t attribute_getKey(attribute_pt attribute, char **key) {
	*key = attribute->key;
	return CELIX_SUCCESS;
}

celix_status_t attribute_getValue(attribute_pt attribute, char **value) {
	*value = attribute->value;
	return CELIX_SUCCESS;
}
