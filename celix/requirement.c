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
 * requirement.c
 *
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "requirement.h"
#include "version_range.h"
#include "attribute.h"

struct requirement {
	char * targetName;
	VERSION_RANGE versionRange;
	HASH_MAP attributes;
	HASH_MAP directives;
};

REQUIREMENT requirement_create(HASH_MAP directives, HASH_MAP attributes) {
	REQUIREMENT requirement = (REQUIREMENT) malloc(sizeof(*requirement));

	requirement->attributes = attributes;
	requirement->directives = directives;

	requirement->versionRange = versionRange_createInfiniteVersionRange();
	ATTRIBUTE versionAttribute = (ATTRIBUTE) hashMap_get(attributes, "version");
	if (versionAttribute != NULL) {
		requirement->versionRange = versionRange_parse(versionAttribute->value);
	}
	ATTRIBUTE serviceAttribute = (ATTRIBUTE) hashMap_get(attributes, "service");
	requirement->targetName = serviceAttribute->value;

	return requirement;
}

VERSION_RANGE requirement_getVersionRange(REQUIREMENT requirement) {
	return requirement->versionRange;
}

char * requirement_getTargetName(REQUIREMENT requirement) {
	return requirement->targetName;
}

bool requirement_isSatisfied(REQUIREMENT requirement, CAPABILITY capability) {
	versionRange_isInRange(requirement_getVersionRange(requirement), capability_getVersion(capability));
}
