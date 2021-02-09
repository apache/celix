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


#ifndef VERSION_RANGE_H_
#define VERSION_RANGE_H_

/**
 * @defgroup VersionRange Version Range functions
 * @ingroup framework
 * @{
 */

#include "celixbool.h"
#include "celix_errno.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif
    
/**
 * Type definition for the version_range_pt abstract data type.
 */
typedef struct celix_version_range *version_range_pt;

/**
 * Creates a new <code>version_range_pt</code>.
 *
 * @param low Lower bound version
 * @param isLowInclusive True if lower bound should be included in the range
 * @param high Upper bound version
 * @param isHighInclusive True if upper bound should be included in the range
 * @param versionRange The created range
 * @return Status code indication failure or success:
 *         - CELIX_SUCCESS when no errors are encountered.
 *         - CELIX_ENOMEM If allocating memory for <code>versionRange</code> failed.
 */
celix_status_t
versionRange_createVersionRange(version_pt low, bool isLowInclusive, version_pt high, bool isHighInclusive,
                                version_range_pt *versionRange);

/**
 * Creates an infinite version range using ::version_createEmptyVersion for the low version,
 *     NULL for the high version and high and low inclusive set to true.
 *
 * @param range The created range
 * @return Status code indication failure or success:
 *         - CELIX_SUCCESS when no errors are encountered.
 *         - CELIX_ENOMEM If allocating memory for <code>range</code> failed.
 */
celix_status_t versionRange_createInfiniteVersionRange(version_range_pt *range);

celix_status_t versionRange_destroy(version_range_pt range);

/**
 * Determine if the specified version is part of the version range or not.
 *
 * @param versionRange The range to check <code>version</code> against.
 * @param version The version to check.
 * @param inRange True if the specified version is included in this version range, false otherwise.
 * @return Status code indication failure or success:
 *         - CELIX_SUCCESS when no errors are encountered.
 */
celix_status_t versionRange_isInRange(version_range_pt versionRange, version_pt version, bool *inRange);

/**
 * Determines whether the lower bound is included in the given range
 *
 * @param versionRange The range to check
 * @param isLowInclusive is set to true in case, the lower bound the lower bound is included, otherwise false
 * @return Status code indication failure or success:
 *      - CELIX_SUCCESS when no errors are encountered.
 *      - CELIX_ILLEGAL_ARGUMENT in case the versionRange is NULL
 */
celix_status_t versionRange_isLowInclusive(version_range_pt versionRange, bool *isLowInclusive);

/**
 * Determines whether the higher bound is included in the given range
 *
 * @param versionRange The range to check
 * @param isHighInclusive is set to true in case, the lower bound the higher bound is included, otherwise false
 * @return Status code indication failure or success:
 *      - CELIX_SUCCESS when no errors are encountered.
 *      - CELIX_ILLEGAL_ARGUMENT in case the versionRange is NULL
 */
celix_status_t versionRange_isHighInclusive(version_range_pt versionRange, bool *isHighInclusive);

/**
 * Retrieves whether the lower bound version from the given range
 *
 * @param versionRange The range
 * @param highVersion is set to the lower bound version
 * @return Status code indication failure or success:
 *      - CELIX_SUCCESS when no errors are encountered.
 *      - CELIX_ILLEGAL_ARGUMENT in case the versionRange is NULL
 */
celix_status_t versionRange_getLowVersion(version_range_pt versionRange, version_pt *lowVersion);

/**
 * Retrieves whether the upper bound version from the given range
 *
 * @param versionRange The range
 * @param highVersion is set to the upper bound version
 * @return Status code indication failure or success:
 *      - CELIX_SUCCESS when no errors are encountered.
 *      - CELIX_ILLEGAL_ARGUMENT in case the versionRange is NULL
 */
celix_status_t versionRange_getHighVersion(version_range_pt versionRange, version_pt *highVersion);

/**
 * Parses a version range from the specified string.
 *
 * <p>
 * Here is the grammar for version range strings.
 *
 * <pre>
 * version-range ::= interval | atleast
 * interval ::= ( '[' | '(' ) floor ',' ceiling ( ']' | ')' )
 * atleast ::= version
 * floor ::= version
 * ceiling ::= version
 * </pre>
 *
 * @param rangeStr String representation of the version range.
 * @param range The created version_range_pt.
 * @return Status code indication failure or success:
 *         - CELIX_SUCCESS when no errors are encountered.
 *         - CELIX_ENOMEM If allocating memory for <code>versionRange</code> failed.
 *         - CELIX_ILLEGAL_ARGUMENT If the numerical components are negative,
 *               the qualifier string is invalid or <code>versionStr</code> is impropertly formatted.
 */
celix_status_t versionRange_parse(const char *rangeStr, version_range_pt *range);

/**
 * Returns the LDAP filter for a version range. Caller is owner of the returned string.
 *
 * @param range                         The version range used as input for the LDAP filer
 * @param serviceVersionPropertyName    The service version name to be used in the filter (i.e. service.version)
 * @return LDAP filter string if valid, NULL otherwise
 */
char* versionRange_createLDAPFilter(version_range_pt range, const char *serviceVersionAttributeName);

/**
 * construct a LDAP filter for the provided version range.
 * The string will be created in the provided buffer, if the buffer is big enough.
 *
 * @return True if parse successful, False otherwise.
 */
bool versionRange_createLDAPFilterInPlace(version_range_pt range, const char *serviceVersionAttributeName, char* buffer, size_t bufferLength);


#ifdef __cplusplus
}
#endif

#endif /* VERSION_RANGE_H_ */
