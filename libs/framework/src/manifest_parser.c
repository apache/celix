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
 * manifest_parser.c
 *
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>

#include "celix_utils.h"
#include "celix_constants.h"
#include "manifest_parser.h"
#include "celix_errno.h"
#include "celix_log.h"

struct manifestParser {
    module_pt owner;
    manifest_pt manifest;
    celix_version_t* bundleVersion;
    // TODO: Implement Requirement-Capability-Model using RCM library
};

celix_status_t manifestParser_create(module_pt owner, manifest_pt manifest, manifest_parser_pt *manifest_parser) {
	celix_status_t status;
	manifest_parser_pt parser;

	status = CELIX_SUCCESS;
	parser = (manifest_parser_pt) malloc(sizeof(*parser));
	if (parser) {
		const char * bundleVersion = NULL;
		parser->manifest = manifest;
		parser->owner = owner;

		bundleVersion = manifest_getValue(manifest, CELIX_FRAMEWORK_BUNDLE_VERSION);
		if (bundleVersion != NULL) {
                    parser->bundleVersion = celix_version_createVersionFromString(bundleVersion);
		} else {
                    parser->bundleVersion = celix_version_createEmptyVersion();
		}

		*manifest_parser = parser;

		status = CELIX_SUCCESS;
	} else {
		status = CELIX_ENOMEM;
	}

	framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot create manifest parser");

	return status;
}

celix_status_t manifestParser_destroy(manifest_parser_pt mp) {
	celix_version_destroy(mp->bundleVersion);
	mp->bundleVersion = NULL;
	mp->manifest = NULL;
	mp->owner = NULL;

	free(mp);

	return CELIX_SUCCESS;
}

static celix_status_t manifestParser_getDuplicateEntry(manifest_parser_pt parser, const char* entryName, char **result) {
    const char *val = manifest_getValue(parser->manifest, entryName);
    if (result != NULL && val == NULL) {
        *result = NULL;
    } else if (result != NULL) {
        *result = celix_utils_strdup(val);
    }
    return CELIX_SUCCESS;
}

celix_status_t manifestParser_getAndDuplicateGroup(manifest_parser_pt parser, char **group) {
    return manifestParser_getDuplicateEntry(parser, CELIX_FRAMEWORK_BUNDLE_GROUP, group);
}

celix_status_t manifestParser_getAndDuplicateSymbolicName(manifest_parser_pt parser, char **symbolicName) {
    return manifestParser_getDuplicateEntry(parser, CELIX_FRAMEWORK_BUNDLE_SYMBOLICNAME, symbolicName);
}

celix_status_t manifestParser_getAndDuplicateName(manifest_parser_pt parser, char **name) {
    return manifestParser_getDuplicateEntry(parser, CELIX_FRAMEWORK_BUNDLE_NAME, name);
}

celix_status_t manifestParser_getAndDuplicateDescription(manifest_parser_pt parser, char **description) {
    return manifestParser_getDuplicateEntry(parser, CELIX_FRAMEWORK_BUNDLE_DESCRIPTION, description);
}

celix_status_t manifestParser_getBundleVersion(manifest_parser_pt parser, celix_version_t **version) {
    *version = celix_version_copy(parser->bundleVersion);
    return *version == NULL ? CELIX_ENOMEM : CELIX_SUCCESS;
}