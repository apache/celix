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
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>

#include "version_range_private.h"

celix_status_t versionRange_createVersionRange(version_pt low, bool isLowInclusive,
			version_pt high, bool isHighInclusive, version_range_pt *range) {
	celix_status_t status = CELIX_SUCCESS;
	*range = (version_range_pt) malloc(sizeof(**range));
	if (!*range) {
		status = CELIX_ENOMEM;
	} else {
		(*range)->low = low;
		(*range)->isLowInclusive = isLowInclusive;
		(*range)->high = high;
		(*range)->isHighInclusive = isHighInclusive;
	}

	return status;
}

celix_status_t versionRange_destroy(version_range_pt range) {
    if (range->high != NULL) {
        version_destroy(range->high);
    }
    if (range->low != NULL) {
        version_destroy(range->low);
    }

	range->high = NULL;
	range->isHighInclusive = false;
	range->low = NULL;
	range->isLowInclusive = false;

	free(range);

	return CELIX_SUCCESS;
}

celix_status_t versionRange_createInfiniteVersionRange(version_range_pt *range) {
	celix_status_t status;

	version_pt version = NULL;
	status = version_createEmptyVersion(&version);
	if (status == CELIX_SUCCESS) {
		status = versionRange_createVersionRange(version, true, NULL, true, range);
	}

	return status;
}

celix_status_t versionRange_isInRange(version_range_pt versionRange, version_pt version, bool *inRange) {
	celix_status_t status;
	if (versionRange->high == NULL) {
		int cmp;
		status = version_compareTo(version, versionRange->low, &cmp);
		if (status == CELIX_SUCCESS) {
			*inRange = (cmp >= 0);
		}
	} else if (versionRange->isLowInclusive && versionRange->isHighInclusive) {
		int low, high;
		status = version_compareTo(version, versionRange->low, &low);
		if (status == CELIX_SUCCESS) {
			status = version_compareTo(version, versionRange->high, &high);
			if (status == CELIX_SUCCESS) {
				*inRange = (low >= 0) && (high <= 0);
			}
		}
	} else if (versionRange->isHighInclusive) {
		int low, high;
		status = version_compareTo(version, versionRange->low, &low);
		if (status == CELIX_SUCCESS) {
			status = version_compareTo(version, versionRange->high, &high);
			if (status == CELIX_SUCCESS) {
				*inRange = (low > 0) && (high <= 0);
			}
		}
	} else if (versionRange->isLowInclusive) {
		int low, high;
		status = version_compareTo(version, versionRange->low, &low);
		if (status == CELIX_SUCCESS) {
			status = version_compareTo(version, versionRange->high, &high);
			if (status == CELIX_SUCCESS) {
				*inRange = (low >= 0) && (high < 0);
			}
		}
	} else {
		int low, high;
		status = version_compareTo(version, versionRange->low, &low);
		if (status == CELIX_SUCCESS) {
			status = version_compareTo(version, versionRange->high, &high);
			if (status == CELIX_SUCCESS) {
				*inRange = (low > 0) && (high < 0);
			}
		}
	}

	return status;
}

celix_status_t versionRange_getLowVersion(version_range_pt versionRange, version_pt *lowVersion) {
    celix_status_t status = CELIX_SUCCESS;

    if (versionRange == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }
    else {
        *lowVersion = versionRange->low;
    }

    return status;
}

celix_status_t versionRange_isLowInclusive(version_range_pt versionRange, bool *isLowInclusive) {
    celix_status_t status = CELIX_SUCCESS;

    if (versionRange == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }
    else {
        *isLowInclusive = versionRange->isLowInclusive;
    }

    return status;
}

celix_status_t versionRange_getHighVersion(version_range_pt versionRange, version_pt *highVersion) {
    celix_status_t status = CELIX_SUCCESS;

    if (versionRange == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }
    else {
        *highVersion = versionRange->high;
    }

    return status;
}

celix_status_t versionRange_isHighInclusive(version_range_pt versionRange, bool *isHighInclusive) {
    celix_status_t status = CELIX_SUCCESS;

    if (versionRange == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }
    else {
        *isHighInclusive = versionRange->isHighInclusive;
    }

    return status;
}


celix_status_t versionRange_parse(const char * rangeStr, version_range_pt *range) {
	celix_status_t status;
	if (strchr(rangeStr, ',') != NULL) {
			int vlowL = strcspn(rangeStr+1, ",");
			char * vlow = (char *) malloc(sizeof(char) * (vlowL + 1));
			if (!vlow) {
				status = CELIX_ENOMEM;
			} else {
				int vhighL;
				char * vhigh;
				vlow = strncpy(vlow, rangeStr+1, vlowL);
				vlow[vlowL] = '\0';
				vhighL = strlen(rangeStr+1) - vlowL - 2;
				vhigh = (char *) malloc(sizeof(char) * (vhighL+1));
				if (!vhigh) {
					status = CELIX_ENOMEM;
				} else {					
					version_pt versionLow = NULL;
					int rangeL = strlen(rangeStr);
					char start = rangeStr[0];
					char end = rangeStr[rangeL-1];

					vhigh = strncpy(vhigh, rangeStr+vlowL+2, vhighL);
					vhigh[vhighL] = '\0';
					status = version_createVersionFromString(vlow, &versionLow);
					if (status == CELIX_SUCCESS) {
						version_pt versionHigh = NULL;
						status = version_createVersionFromString(vhigh, &versionHigh);
						if (status == CELIX_SUCCESS) {
							status = versionRange_createVersionRange(
									versionLow,
									start == '[',
									versionHigh,
									end ==']',
									range
								);
						}
					}
					free(vhigh);
				}
				free(vlow);

		}
	} else {
		version_pt version = NULL;
		status = version_createVersionFromString(rangeStr, &version);
		if (status == CELIX_SUCCESS) {
			status = versionRange_createVersionRange(version, true, NULL, false, range);
		}
	}

	return status;
}

