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


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "version_private.h"
#include "version_range.h"
#include "celix_version_range.h"
#include "version_range_private.h"
#include "celix_version_range.h"

celix_status_t versionRange_createVersionRange(version_pt low, bool isLowInclusive,
            version_pt high, bool isHighInclusive, version_range_pt *range) {
    *range = celix_versionRange_createVersionRange(low, isLowInclusive, high, isHighInclusive);
    return CELIX_SUCCESS;
}

celix_status_t versionRange_destroy(version_range_pt range) {
    celix_versionRange_destroy(range);
    return CELIX_SUCCESS;
}

celix_status_t versionRange_createInfiniteVersionRange(version_range_pt *range) {
    *range = celix_versionRange_createInfiniteVersionRange();
    return CELIX_SUCCESS;
}

celix_status_t versionRange_isInRange(version_range_pt versionRange, version_pt version, bool *inRange) {
    if (versionRange != NULL && version != NULL) {
        *inRange = celix_versionRange_isInRange(versionRange, version);
        return CELIX_SUCCESS;
    } else {
        return CELIX_ILLEGAL_ARGUMENT;
    }
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
    *range = celix_versionRange_parse(rangeStr);
    if (*range == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    } else {
        return CELIX_SUCCESS;
    }
}

char* versionRange_createLDAPFilter(version_range_pt range, const char *serviceVersionAttributeName) {
    return celix_versionRange_createLDAPFilter(range, serviceVersionAttributeName);
}



bool versionRange_createLDAPFilterInPlace(version_range_pt range, const char *serviceVersionAttributeName, char* buffer, size_t bufferLength) {
    return celix_versionRange_createLDAPFilterInPlace(range, serviceVersionAttributeName, buffer, bufferLength);
}

celix_version_range_t* celix_versionRange_createVersionRange(celix_version_t* low, bool isLowInclusive, celix_version_t* high, bool isHighInclusive) {
    assert(low != high);
    celix_version_range_t* range = malloc(sizeof(*range));
    range->low = low;
    range->isLowInclusive = isLowInclusive;
    range->high = high;
    range->isHighInclusive = isHighInclusive;
    return range;
}


celix_version_range_t* celix_versionRange_createInfiniteVersionRange() {
    return celix_versionRange_createVersionRange(celix_version_createEmptyVersion(), true, NULL, true);
}

void celix_versionRange_destroy(celix_version_range_t* range) {
    if (range->high != NULL) {
        celix_version_destroy(range->high);
    }
    if (range->low != NULL) {
        celix_version_destroy(range->low);
    }
    free(range);
}


bool celix_versionRange_isInRange(const celix_version_range_t* versionRange, const celix_version_t* version) {
    bool inRange = false;
    int high = 0;
    int low = 0;
    if (versionRange->high != NULL) {
        high = celix_version_compareTo(version, versionRange->high);
    }
    if (versionRange->low != NULL) {
        low = celix_version_compareTo(version, versionRange->low);
    }

    if (versionRange->high == NULL) {
        inRange = (low >= 0);
    } else if (versionRange->isLowInclusive && versionRange->isHighInclusive) {
        inRange = (low >= 0) && (high <= 0);
    } else if (versionRange->isHighInclusive) {
        inRange = (low > 0) && (high <= 0);
    } else if (versionRange->isLowInclusive) {
        inRange = (low >= 0) && (high < 0);
    } else {
        inRange = (low > 0) && (high < 0);
    }

    return inRange;
}


bool celix_versionRange_isLowInclusive(const celix_version_range_t* versionRange) {
    return versionRange->isLowInclusive;
}


bool celix_versionRange_isHighInclusive(const celix_version_range_t* versionRange) {
    return versionRange->isHighInclusive;
}


celix_version_t* celix_versionRange_getLowVersion(const celix_version_range_t* versionRange) {
    return versionRange->low;
}


celix_version_t* celix_versionRange_getHighVersion(const celix_version_range_t* versionRange) {
    return versionRange->high;
}

celix_version_range_t* celix_versionRange_parse(const char *rangeString) {
    celix_version_range_t* range = NULL;
    if (strchr(rangeString, ',') != NULL) {
        int vlowL = strcspn(rangeString+1, ",");
        char * vlow = malloc(sizeof(char) * (vlowL + 1));
        int vhighL;
        char * vhigh;
        vlow = strncpy(vlow, rangeString+1, vlowL);
        vlow[vlowL] = '\0';
        vhighL = strlen(rangeString+1) - vlowL - 2;
        vhigh = (char *) malloc(sizeof(char) * (vhighL+1));
        int rangeL = strlen(rangeString);
        char start = rangeString[0];
        char end = rangeString[rangeL-1];

        vhigh = strncpy(vhigh, rangeString+vlowL+2, vhighL);
        vhigh[vhighL] = '\0';
        celix_version_t* versionLow = celix_version_createVersionFromString(vlow);
        if (versionLow != NULL) {
            celix_version_t* versionHigh = celix_version_createVersionFromString(vhigh);
            if (versionHigh != NULL) {
                range = celix_versionRange_createVersionRange(
                        versionLow,
                        start == '[',
                        versionHigh,
                        end ==']'
                );
            }
        }
        free(vhigh);
        free(vlow);
    } else {
        celix_version_t* version = celix_version_createVersionFromString(rangeString);
        if (version != NULL) {
            range = celix_versionRange_createVersionRange(version, true, NULL, false);
        }
    }

    return range;
}

char* celix_versionRange_createLDAPFilter(const celix_version_range_t* range, const char *serviceVersionAttributeName) {
    char *output = NULL;

    int ret = -1;
    if(range->high == NULL) {
        ret = asprintf(&output, "(&(%s%s%i.%i.%i))",
                       serviceVersionAttributeName, range->isLowInclusive ? ">=" : ">", range->low->major,
                       range->low->minor, range->low->micro);
    } else {
        ret = asprintf(&output, "(&(%s%s%i.%i.%i)(%s%s%i.%i.%i))",
                       serviceVersionAttributeName, range->isLowInclusive ? ">=" : ">", range->low->major,
                       range->low->minor, range->low->micro,
                       serviceVersionAttributeName, range->isHighInclusive ? "<=" : "<", range->high->major,
                       range->high->minor, range->high->micro);
    }

    if (ret < 0) {
        return NULL;
    }

    return output;
}

bool celix_versionRange_createLDAPFilterInPlace(const celix_version_range_t* range, const char *serviceVersionAttributeName, char* buffer, size_t bufferLength) {
    if(buffer == NULL || bufferLength == 0) {
        return false;
    }

    const char* format = "(&(%s%s%i.%i.%i)(%s%s%i.%i.%i))";
    if(range->high == NULL) {
        format = "(&(%s%s%i.%i.%i))";
    }

    // check if buffer is long enough
    int size = 0;
    if(range->high == NULL) {
        size = snprintf(NULL, 0, format,
                        serviceVersionAttributeName, range->isLowInclusive ? ">=" : ">", range->low->major,
                        range->low->minor, range->low->micro);
    } else {
        size = snprintf(NULL, 0, format,
                        serviceVersionAttributeName, range->isLowInclusive ? ">=" : ">", range->low->major,
                        range->low->minor, range->low->micro,
                        serviceVersionAttributeName, range->isHighInclusive ? "<=" : "<", range->high->major,
                        range->high->minor, range->high->micro);
    }

    if(size >= bufferLength || size < 0) {
        return false;
    }

    // write contents into buffer
    if(range->high == NULL) {
        size = snprintf(buffer, bufferLength, format,
                        serviceVersionAttributeName, range->isLowInclusive ? ">=" : ">", range->low->major,
                        range->low->minor, range->low->micro);
    } else {
        size = snprintf(buffer, bufferLength, format,
                        serviceVersionAttributeName, range->isLowInclusive ? ">=" : ">", range->low->major,
                        range->low->minor, range->low->micro,
                        serviceVersionAttributeName, range->isHighInclusive ? "<=" : "<", range->high->major,
                        range->high->minor, range->high->micro);
    }

    if(size >= bufferLength || size < 0) {
        return false;
    }

    return true;
}