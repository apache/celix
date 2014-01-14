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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>

#include "version_range_private.h"
#include "celix_log.h"

apr_status_t versionRange_destroy(void *rangeP);

celix_status_t versionRange_createVersionRange(apr_pool_t *pool, version_pt low, bool isLowInclusive,
			version_pt high, bool isHighInclusive, version_range_pt *range) {
	celix_status_t status = CELIX_SUCCESS;
	*range = (version_range_pt) apr_palloc(pool, sizeof(**range));
	if (!*range) {
		status = CELIX_ENOMEM;
	} else {
		apr_pool_pre_cleanup_register(pool, *range, versionRange_destroy);

		(*range)->low = low;
		(*range)->isLowInclusive = isLowInclusive;
		(*range)->high = high;
		(*range)->isHighInclusive = isHighInclusive;
	}

	framework_logIfError(logger, status, NULL, "Cannot create version range");

	return status;
}

apr_status_t versionRange_destroy(void *rangeP) {
	version_range_pt range = rangeP;
	range->high = NULL;
	range->isHighInclusive = false;
	range->low = NULL;
	range->isLowInclusive = false;
	return APR_SUCCESS;
}

celix_status_t versionRange_createInfiniteVersionRange(apr_pool_t *pool, version_range_pt *range) {
	celix_status_t status = CELIX_SUCCESS;

	version_pt version = NULL;
	status = version_createEmptyVersion(pool, &version);
	if (status == CELIX_SUCCESS) {
		status = versionRange_createVersionRange(pool, version, true, NULL, true, range);
	}

	framework_logIfError(logger, status, NULL, "Cannot create infinite range");

	return status;
}

celix_status_t versionRange_isInRange(version_range_pt versionRange, version_pt version, bool *inRange) {
	celix_status_t status = CELIX_SUCCESS;
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

	framework_logIfError(logger, status, NULL, "Cannot check if version in in range");

	return status;
}

celix_status_t versionRange_parse(apr_pool_t *pool, char * rangeStr, version_range_pt *range) {
	celix_status_t status = CELIX_SUCCESS;
	if (strchr(rangeStr, ',') != NULL) {
		apr_pool_t *spool;
		apr_status_t aprStatus = apr_pool_create(&spool, pool);
		if (aprStatus != APR_SUCCESS) {
			status = CELIX_ILLEGAL_STATE;
		} else {
			int vlowL = strcspn(rangeStr+1, ",");
			char * vlow = (char *) apr_palloc(spool, sizeof(*vlow * vlowL));
			if (!vlow) {
				status = CELIX_ENOMEM;
			} else {
				int vhighL;
				char * vhigh;
				vlow = strncpy(vlow, rangeStr+1, vlowL);
				vhighL = strlen(rangeStr+1) - vlowL - 2;
				vhigh = (char *) apr_palloc(spool, sizeof(*vhigh * vhighL));
				if (!vhigh) {
					status = CELIX_ENOMEM;
				} else {					
					version_pt versionLow = NULL;
					int rangeL = strlen(rangeStr);
					char start = rangeStr[0];
					char end = rangeStr[rangeL-1];

					vhigh = strncpy(vhigh, rangeStr+vlowL+2, vhighL);
					
					status = version_createVersionFromString(pool, vlow, &versionLow);
					if (status == CELIX_SUCCESS) {
						version_pt versionHigh = NULL;
						status = version_createVersionFromString(pool, vhigh, &versionHigh);
						if (status == CELIX_SUCCESS) {
							status = versionRange_createVersionRange(pool,
									versionLow,
									start == '[',
									versionHigh,
									end ==']',
									range
								);
						}
					}
				}
			}
			apr_pool_destroy(spool);
		}
	} else {
		version_pt version = NULL;
		status = version_createVersionFromString(pool, rangeStr, &version);
		if (status == CELIX_SUCCESS) {
			status = versionRange_createVersionRange(pool, version, true, NULL, false, range);
		}
	}

	framework_logIfError(logger, status, NULL, "Cannot parse version range");

	return status;
}
