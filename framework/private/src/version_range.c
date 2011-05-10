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
 * version_range.c
 *
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */

#include <stdlib.h>
#include <string.h>

#include "version_range.h"

struct versionRange {
	VERSION low;
	bool isLowInclusive;
	VERSION high;
	bool isHighInclusive;

};

VERSION_RANGE versionRange_createVersionRange(VERSION low, bool isLowInclusive,
			VERSION high, bool isHighInclusive) {
	VERSION_RANGE range = (VERSION_RANGE) malloc(sizeof(*range));

	range->low = low;
	range->isLowInclusive = isLowInclusive;
	range->high = high;
	range->isHighInclusive = isHighInclusive;

	return range;
}

void versionRange_destroy(VERSION_RANGE range) {
	if (range->high != NULL) {
		version_destroy(range->high);
	}
	if (range->low != NULL) {
		version_destroy(range->low);
	}
	free(range);
}

VERSION_RANGE versionRange_createInfiniteVersionRange() {
	VERSION_RANGE range = (VERSION_RANGE) malloc(sizeof(*range));

	range->low = version_createEmptyVersion();
	range->isLowInclusive = true;
	range->high = NULL;
	range->isHighInclusive = true;

	return range;
}

bool versionRange_isInRange(VERSION_RANGE versionRange, VERSION version) {
	if (versionRange->high == NULL) {
		return (version_compareTo(version, versionRange->low) >= 0);
	} else if (versionRange->isLowInclusive && versionRange->isHighInclusive) {
		return (version_compareTo(version, versionRange->low) >= 0)
				&& (version_compareTo(version, versionRange->high) <= 0);
	} else if (versionRange->isHighInclusive) {
		return (version_compareTo(version, versionRange->low) > 0)
				&& (version_compareTo(version, versionRange->high) <= 0);
	} else if (versionRange->isLowInclusive) {
		return (version_compareTo(version, versionRange->low) >= 0)
				&& (version_compareTo(version, versionRange->high) < 0);
	} else {
		return (version_compareTo(version, versionRange->low) > 0)
				&& (version_compareTo(version, versionRange->high) < 0);
	}
}

bool versionRange_intersects(VERSION_RANGE versionRange, VERSION_RANGE toCompare) {
	bool isFloorLessThanCeiling = false;
	if ((versionRange->high == NULL)
			|| (version_compareTo(versionRange->high, toCompare->low) > 0)
			|| ((version_compareTo(versionRange->high, toCompare->low) == 0)
				&& versionRange->isHighInclusive && toCompare->isLowInclusive)) {
		isFloorLessThanCeiling = true;
	}
	bool isCeilingGreaterThanFloor = false;
	if ((toCompare->high == NULL)
			|| (version_compareTo(versionRange->low, toCompare->high) < 0)
			|| ((version_compareTo(versionRange->low, toCompare->high) == 0)
				&& versionRange->isLowInclusive && toCompare->isHighInclusive)) {
		isCeilingGreaterThanFloor = true;
	}
	return isFloorLessThanCeiling && isCeilingGreaterThanFloor;
}

VERSION_RANGE versionRange_parse(char * range) {
	if (strchr(range, ',') != NULL) {
		int vlowL = strcspn(range+1, ",");
		char * vlow = (char *) malloc(sizeof(*vlow * vlowL));
		vlow = strncpy(vlow, range+1, vlowL);
		int vhighL = strlen(range+1) - vlowL - 2;
		char * vhigh = (char *) malloc(sizeof(*vhigh * vhighL));
		vhigh = strncpy(vhigh, range+vlowL+2, vhighL);

		int rangeL = strlen(range);
		char start = range[0];
		char end = range[rangeL-1];

		return versionRange_createVersionRange(
				version_createVersionFromString(vlow),
				start == '[',
				version_createVersionFromString(vhigh),
				end ==']'
			);
	} else {
		return versionRange_createVersionRange(version_createVersionFromString(range), true, NULL, false);
	}
}
