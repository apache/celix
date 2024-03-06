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


#ifndef CELIX_VERSION_RANGE_H
#define CELIX_VERSION_RANGE_H

#include <stdbool.h>

#include "celix_version_range_type.h"
#include "celix_version_type.h"
#include "celix_utils_export.h"
#include "celix_errno.h"
#include "celix_cleanup.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a new <code>celix_version_range_t*</code>.
 *
 * @param low Lower bound version
 * @param isLowInclusive True if lower bound should be included in the range
 * @param high Upper bound version
 * @param isHighInclusive True if upper bound should be included in the range
 * @param versionRange The created range
 * @return The service range or NULL if the service range could not be created
 */
CELIX_UTILS_EXPORT celix_version_range_t*
celix_versionRange_createVersionRange(celix_version_t* low, bool isLowInclusive, celix_version_t* high, bool isHighInclusive);

/**
 * Creates an infinite version range using ::version_createEmptyVersion for the low version,
 *     NULL for the high version and high and low inclusive set to true.
 *
 * @return The created range
 */
CELIX_UTILS_EXPORT celix_version_range_t* celix_versionRange_createInfiniteVersionRange();

CELIX_UTILS_EXPORT void celix_versionRange_destroy(celix_version_range_t* range);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_version_range_t, celix_versionRange_destroy)

/**
 * Determine if the specified version is part of the version range or not.
 *
 * @param versionRange The range to check <code>version</code> against.
 * @param version The version to check.
 * @return  True if the specified version is included in this version range, false otherwise.
 */
CELIX_UTILS_EXPORT bool celix_versionRange_isInRange(const celix_version_range_t* versionRange, const celix_version_t* version);

/**
 * Determines whether the lower bound is included in the given range
 *
 * @param versionRange The range to check
 * @return true in case, the lower bound the lower bound is included, otherwise false.
 */
CELIX_UTILS_EXPORT bool celix_versionRange_isLowInclusive(const celix_version_range_t* versionRange);

/**
 * Determines whether the higher bound is included in the given range
 *
 * @param versionRange The range to check.
 * @return true in case, the lower bound the higher bound is included, otherwise false.
 */
CELIX_UTILS_EXPORT bool celix_versionRange_isHighInclusive(const celix_version_range_t* versionRange);

/**
 * Retrieves whether the lower bound version from the given range
 *
 * @param versionRange The range
 * @param The lower bound version.
 */
CELIX_UTILS_EXPORT celix_version_t* celix_versionRange_getLowVersion(const celix_version_range_t* versionRange);

/**
 * Retrieves whether the upper bound version from the given range
 *
 * @param versionRange The range
 * @return The upper bound version.
 */
CELIX_UTILS_EXPORT celix_version_t* celix_versionRange_getHighVersion(const celix_version_range_t* versionRange);

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
 * Examples: "[1,2)", "[1.1,1.2)"
 *
 * @param rangeString String representation of the version range.
 * @return The created celix_version_range_t or NULL if the range string was invalid.
 */
CELIX_UTILS_EXPORT celix_version_range_t* celix_versionRange_parse(const char *rangeString);

/**
 * Returns the LDAP filter for a version range. Caller is owner of the returned string.
 *
 * @param range                         The version range used as input for the LDAP filer
 * @param serviceVersionPropertyName    The service version name to be used in the filter (i.e. service.version)
 * @return LDAP filter string if valid, NULL otherwise
 */
CELIX_UTILS_EXPORT char* celix_versionRange_createLDAPFilter(const celix_version_range_t* range, const char *serviceVersionAttributeName);

/**
 * construct a LDAP filter for the provided version range.
 * The string will be created in the provided buffer, if the buffer is big enough.
 *
 * @return True if parse successful, False otherwise.
 */
CELIX_UTILS_EXPORT bool celix_versionRange_createLDAPFilterInPlace(const celix_version_range_t* range, const char *serviceVersionAttributeName, char* buffer, size_t bufferLength);


#ifdef __cplusplus
}
#endif

#endif /* CELIX_VERSION_RANGE_H */
