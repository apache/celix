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

#ifndef CELIX_CELIX_VERSION_H
#define CELIX_CELIX_VERSION_H

#include <stdbool.h>
#include <stddef.h>

#include "celix_version_type.h"
#include "celix_cleanup.h"
#include "celix_utils_export.h"
#include "celix_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file celix_version.h
 * @brief Header file for the Celix Version API.
 *
 * The Celix Version API provides a means for storing and manipulating version information, which consists of
 * three non-negative integers for the major, minor, and micro version, and an optional string for the qualifier.
 * This implementation is based on the Semantic Versioning specification (SemVer).
 * Functions are provided for creating and destroying version objects, comparing versions, and extracting the individual version components.
 */

/**
 * @brief Create a new celix_version_t* using the supplied arguments.
 *
 * If the return is NULL, an error message is logged to celix_err.
 *
 * @param[in] major Major component of the version identifier.
 * @param[in] minor Minor component of the version identifier.
 * @param[in] micro Micro component of the version identifier.
 * @param[in] qualifier Qualifier component of the version identifier. If
 *        <code>NULL</code> is specified, then the qualifier will be set to
 *        the empty string.
 * @return The created version or NULL if the input was incorrect or memory could not be allocated.
 */
CELIX_UTILS_EXPORT celix_version_t* celix_version_create(int major, int minor, int micro, const char* qualifier);

/**
 * @brief Destroy a celix_version_t*.
 * @param version The version to destroy.
 */
CELIX_UTILS_EXPORT void celix_version_destroy(celix_version_t* version);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_version_t, celix_version_destroy)

/**
 * @brief Create a copy of <code>version</code>.
 *
 * If the provided version is NULL a NULL pointer is returned.
 * If the return is NULL, an error message is logged to celix_err.
 *
 * @param[in] version The version to copy
 * @return the copied version
 */
CELIX_UTILS_EXPORT celix_version_t* celix_version_copy(const celix_version_t* version);

/**
 * @brief Create a version identifier from the specified string.
 *
 * <p>
 * Here is the grammar for version strings.
 *
 * <pre>
 * version ::= major('.'minor('.'micro('.'qualifier)?)?)?
 * major ::= digit+
 * minor ::= digit+
 * micro ::= digit+
 * qualifier ::= (alpha|digit|'_'|'-')+
 * digit ::= [0..9]
 * alpha ::= [a..zA..Z]
 * </pre>
 *
 * There must be no whitespace in version.
 *
 * If the return is NULL, an error message is logged to celix_err.
 *
 * @param[in] versionStr String representation of the version identifier.
 * @return The created version or NULL if the input was invalid or memory could not be allocated.
 */
CELIX_UTILS_EXPORT celix_version_t* celix_version_createVersionFromString(const char *versionStr);

/**
 * @brief Parse a version string into a version object.
 *
 * <p>
 * Here is the grammar for version strings.
 *
 * <pre>
 * version ::= major('.'minor('.'micro('.'qualifier)?)?)?
 * major ::= digit+
 * minor ::= digit+
 * micro ::= digit+
 * qualifier ::= (alpha|digit|'_'|'-')+
 * digit ::= [0..9]
 * alpha ::= [a..zA..Z]
 * </pre>
 *
 * If the return is NULL, an error message is logged to celix_err.
 *
 * @param[in] versionStr The version string to parse.
 * @param[out] version The parsed version object.
 * @return CELIX_SUCCESS if the version string was parsed successfully, CELIX_ILLEGAL_ARGUMENT if the version string
 *         was invalid, or CELIX_ENOMEM if memory could not be allocated.
 */
CELIX_UTILS_EXPORT celix_status_t celix_version_parse(const char *versionStr, celix_version_t** version);

/**
 * @brief Create empty version "0.0.0".
 */
CELIX_UTILS_EXPORT celix_version_t* celix_version_createEmptyVersion();

/**
 * @brief Gets the major version number of a celix version.
 *
 * @param[in] version The celix version.
 * @return The major version number.
 */
CELIX_UTILS_EXPORT int celix_version_getMajor(const celix_version_t* version);

/**
 * @brief Gets the minor version number of a celix version.
 *
 * @param[in] version The celix version.
 * @return The minor version number.
 */
CELIX_UTILS_EXPORT int celix_version_getMinor(const celix_version_t* version);

/**
 * @brief Gets the micro version number of a celix version.
 *
 * @param[in] version The celix version.
 * @return The micro version number.
 */
CELIX_UTILS_EXPORT int celix_version_getMicro(const celix_version_t* version);

/**
 * @brief Gets the version qualifier of a celix version.
 *
 * @param[in] version The celix version.
 * @return The version qualifier, or an empty string ("") if no qualifier is present.
 */
CELIX_UTILS_EXPORT const char* celix_version_getQualifier(const celix_version_t* version);

/**
 * @brief Compare this <code>Version</code> object to another object.
 *
 * <p>
 * A version is considered to be <b>less than </b> another version if its
 * major component is less than the other version's major component, or the
 * major components are equal and its minor component is less than the other
 * version's minor component, or the major and minor components are equal
 * and its micro component is less than the other version's micro component,
 * or the major, minor and micro components are equal and it's qualifier
 * component is less than the other version's qualifier component (using
 * <code>String.compareTo</code>).
 *
 * <p>
 * A version is considered to be <b>equal to</b> another version if the
 * major, minor and micro components are equal and the qualifier component
 * is equal (using <code>String.compareTo</code>).
 *
 * @param version The <code>celix_version_t*</code> to be compared with <code>compare</code>.
 * @param compare The <code>celix_version_t*</code> to be compared with <code>version</code>.
 * @return A negative integer, zero, or a positive integer if <code>version</code> is
 *         less than, equal to, or greater than the <code>compare</code>.
 */
CELIX_UTILS_EXPORT int celix_version_compareTo(const celix_version_t* version, const celix_version_t* compare);

/**
 * @brief Create a hash of the version
 */
CELIX_UTILS_EXPORT unsigned int celix_version_hash(const celix_version_t* version);

/**
 * @brief Return the string representation of <code>version</code> identifier.
 *
 * The format of the version string will be <code>major.minor.micro</code>
 * if qualifier is the empty string or
 * <code>major.minor.micro.qualifier</code> otherwise.
 *
 * If the return is NULL, an error message is logged to celix_err.
 *
 * @return The string representation of this version identifier.
 * @param version The <code>celix_version_t*</code> to get the string representation from.
 * @return Pointer to the string (char *) in which the result will be placed. Caller is owner of the string.
 *         NULL if memory could not be allocated.
 */
CELIX_UTILS_EXPORT char* celix_version_toString(const celix_version_t* version);

/**
 * @brief Fill a given string with the string representation of the given version.
 *
 * @param[in] version The version to fill the string with.
 * @param[out] str The string to fill.
 * @param[in] strLen The length of the string.
 * @return true if the string was filled successfully, false otherwise.
 */
CELIX_UTILS_EXPORT bool celix_version_fillString(const celix_version_t* version, char *str, size_t strLen);

/**
 * @brief Check if two versions are semantically compatible.
 *
 * <p>
 * The user version is compatible with the provider version if the provider version is in the range
 * [user_version, next_major_from_user_version)
 *
 * @param version The user <code>celix_version_t*</code> .
 * @param version The reference provider <code>celix_version_t*</code> .
 * @return Boolean indicating if the versions are compatible
 */
CELIX_UTILS_EXPORT bool celix_version_isCompatible(const celix_version_t* user, const celix_version_t* provider);

/**
 * @brief Check if two versions are semantically compatible.
 *
 * <p>
 * The user version is compatible with the provider version if the provider version is in the range
 * [user_version, next_major_from_user_version)
 *
 * @param version The user <code>celix_version_t*</code> .
 * @param providerMajorVersionPart The major part of the provider version
 * @param provideMinorVersionPart The minor part of the provider version
 * @return Boolean indicating if the versions are compatible
 */
CELIX_UTILS_EXPORT bool celix_version_isUserCompatible(const celix_version_t* user, int providerMajorVersionPart, int provideMinorVersionPart);

/**
 * @brief Compare a provider celix version is with a provided major and minor version. Ignoring the patch version part.
 *
 * @param version The version to compare.
 * @param majorVersionPart The major version part to compare.
 * @param minorVersionPart The minor version part to compare.
 * @return A negative integer, zero, or a positive integer if <code>version</code> is
 *         less than, equal to, or greater than the <code>majorVersionPart</code> and <code>minorVersionPart</code>.
 */
CELIX_UTILS_EXPORT int celix_version_compareToMajorMinor(const celix_version_t* version, int majorVersionPart, int minorVersionPart);

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_VERSION_H
