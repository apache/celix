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
 * version_range.h
 *
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef VERSION_RANGE_H_
#define VERSION_RANGE_H_

/**
 * @defgroup VersionRange Version Range functions
 * @ingroup framework
 * @{
 */

#include "celixbool.h"

#include "version.h"
#include "celix_errno.h"

/**
 * Type definition for the version_range_pt abstract data type.
 */
typedef struct versionRange * version_range_pt;

/**
 * Creates a new <code>version_range_pt</code>.
 *
 * @param low Lower bound version
 * @param isLowInclusive True if lower bound should be included in the range
 * @param high Upper bound version
 * @param isHighInclusive True if upper bound should be included in the range
 * @param versionRange The created range
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ENOMEM If allocating memory for <code>versionRange</code> failed.
 */
celix_status_t versionRange_createVersionRange(version_pt low, bool isLowInclusive, version_pt high, bool isHighInclusive, version_range_pt *versionRange);

/**
 * Creates an infinite version range using ::version_createEmptyVersion for the low version,
 * 	NULL for the high version and high and low inclusive set to true.
 *
 * @param range The created range
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ENOMEM If allocating memory for <code>range</code> failed.
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
 * 		- CELIX_SUCCESS when no errors are encountered.
 */
celix_status_t versionRange_isInRange(version_range_pt versionRange, version_pt version, bool *inRange);

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
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ENOMEM If allocating memory for <code>versionRange</code> failed.
 * 		- CELIX_ILLEGAL_ARGUMENT If the numerical components are negative,
 * 		  	the qualifier string is invalid or <code>versionStr</code> is impropertly formatted.
 */
celix_status_t versionRange_parse(char * rangeStr, version_range_pt *range);

/**
 * @}
 */

#endif /* VERSION_RANGE_H_ */
