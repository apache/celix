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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * The definition of the celix_version_t* abstract data type.
 */
typedef struct celix_version celix_version_t;

/**
 * Creates a new celix_version_t* using the supplied arguments.
 *
 * @param major Major component of the version identifier.
 * @param minor Minor component of the version identifier.
 * @param micro Micro component of the version identifier.
 * @param qualifier Qualifier component of the version identifier. If
 *        <code>null</code> is specified, then the qualifier will be set to
 *        the empty string.
 * @return The created version or NULL if the input was incorrect
 */
celix_version_t* celix_version_createVersion(int major, int minor, int micro, const char* qualifier);

void celix_version_destroy(celix_version_t* version);

/**
 * Creates a copy of <code>version</code>.
 *
 * @param version The version to copy
 * @return the copied version
 */
celix_version_t* celix_version_copy(const celix_version_t* version);

/**
 * Creates a version identifier from the specified string.
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
 * @param versionStr String representation of the version identifier.
 * @return The created version or NULL if the input was invalid.
 */
celix_version_t* celix_version_createVersionFromString(const char *versionStr);

/**
 * The empty version "0.0.0".
 *
 */
celix_version_t* celix_version_createEmptyVersion();

int celix_version_getMajor(const celix_version_t* version);

int celix_version_getMinor(const celix_version_t* version);

int celix_version_getMicro(const celix_version_t* version);

const char* celix_version_getQualifier(const celix_version_t* version);

/**
 * Compares this <code>Version</code> object to another object.
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
int celix_version_compareTo(const celix_version_t* version, const celix_version_t* compare);

/**
 * Creates a hash of the version
 */
unsigned int celix_version_hash(const celix_version_t* version);

/**
 * Returns the string representation of <code>version</code> identifier.
 *
 * <p>
 * The format of the version string will be <code>major.minor.micro</code>
 * if qualifier is the empty string or
 * <code>major.minor.micro.qualifier</code> otherwise.
 *
 * @return The string representation of this version identifier.
 * @param version The <code>celix_version_t*</code> to get the string representation from.
 * @return Pointer to the string (char *) in which the result will be placed. Caller is owner of the string.
 */
char* celix_version_toString(const celix_version_t* version);

/**
 * Check if two versions are semantically compatible.
 *
 * <p>
 * The user version is compatible with the provider version if the provider version is in the range
 * [user_version, next_major_from_user_version)
 *
 * @param version The user <code>celix_version_t*</code> .
 * @param version The reference provider <code>celix_version_t*</code> .
 * @return Boolean indicating if the versions are compatible
 */
bool celix_version_isCompatible(const celix_version_t* user, const celix_version_t* provider);

/**
 * Check if two versions are semantically compatible.
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
bool celix_version_isUserCompatible(const celix_version_t* user, int providerMajorVersionPart, int provideMinorVersionPart);

/**
 * Compare a provider celix version is with a provided major and minor version. Ignoring the patch version part.
 *
 * @param version The version to compare.
 * @param majorVersionPart The major version part to compare.
 * @param minorVersionPart The minor version part to compare.
 * @return A negative integer, zero, or a positive integer if <code>version</code> is
 *         less than, equal to, or greater than the <code>majorVersionPart</code> and <code>minorVersionPart</code>.
 */
int celix_version_compareToMajorMinor(const celix_version_t* version, int majorVersionPart, int minorVersionPart);

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_VERSION_H
