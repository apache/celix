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
