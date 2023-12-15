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

#ifndef __DYN_MESSAGE_H_
#define __DYN_MESSAGE_H_

#include "dyn_type.h"
#include "celix_version.h"
#include "celix_dfi_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Description string
 *
 * Descriptor (message) = HeaderSection AnnotationSection TypesSection MessageSection
 *
 * HeaderSection=
 * ':header\n' [NameValue]*
 * ':annotations\n' [NameValue]*
 * ':types\n' [TypeIdValue]*
 * ':message\n' [MessageIdValue]
 *
 */
typedef struct _dyn_message_type dyn_message_type;


/**
 * @brief Creates a dynamic message type instance according to the given message descriptor stream.
 *
 * The caller is the owner of the dyn_message_type and the dyn_message_type should be freed using dynMessage_destroy.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] descriptor The message descriptor file stream.
 * @param[out] out The dynamic message type instance.
 * @return 0 if successful, 1 otherwise.
 */
CELIX_DFI_EXPORT int dynMessage_parse(FILE *descriptor, dyn_message_type **out);

/**
 * @brief Destroy the dynamic message type instance.
 * @param[in] msg The dynamic message type instance.
 */
CELIX_DFI_EXPORT void dynMessage_destroy(dyn_message_type *msg);

/**
 * @brief Gets the name of the dynamic message type instance.
 *
 * The dynamic message type instance is the owner of the name and the name should not be freed.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] msg The dynamic message type instance.
 * @param[out] name The name of the dynamic message type instance.
 * @return 0 if successful, 1 otherwise.
 */
CELIX_DFI_EXPORT int dynMessage_getName(dyn_message_type *msg, char **name);

/**
 * @brief Gets the version of the given dynamic message type instance.
 *
 * The dynamic message type instance is the owner of the version and the version should not be freed.
 *
 * @param[in] msg The dynamic message type instance.
 * @param[out] version The version of the dynamic message type instance.
 * @return  0 if successful, 1 otherwise.
 */
CELIX_DFI_EXPORT int dynMessage_getVersion(dyn_message_type *msg, celix_version_t** version);

/**
 * @brief Gets the version string of the given dynamic message type instance.
 *
 * The dynamic message type instance is the owner of the version string and the version string should not be freed.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] msg The dynamic message type instance.
 * @param[out] version The version string of the dynamic message type instance.
 * @return  0 if successful, 1 otherwise.
 */
CELIX_DFI_EXPORT int dynMessage_getVersionString(dyn_message_type *msg, char **version);

/**
 * @brief Gets the value corresponding to the specified name, which comes from the header section of the given dynamic message type instance.
 *
 * The dynamic message type instance is the owner of the value and the value should not be freed.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] msg The dynamic message type instance.
 * @param[in] name The name of the value.
 * @param[out] value The value corresponding to the specified name.
 * @return  0 if successful, 1 otherwise.
 */
CELIX_DFI_EXPORT int dynMessage_getHeaderEntry(dyn_message_type *msg, const char *name, char **value);

/**
 * @brief Gets the value corresponding to the specified name, which comes from the annotation section of the given dynamic message type instance.
 *
 * The dynamic message type instance is the owner of the value and the value should not be freed.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] msg The dynamic message type instance.
 * @param[in] name The name of the value.
 * @param[out] value The value corresponding to the specified name.
 * @return  0 if successful, 1 otherwise.
 */
CELIX_DFI_EXPORT int dynMessage_getAnnotationEntry(dyn_message_type *msg, const char *name, char **value);

/**
 * @brief Gets the message type from the given dynamic message type instance.
 *
 * The dynamic message type instance is the owner of the message type and the message type should not be freed.
 *
 * @param[in] msg The dynamic message type instance.
 * @param[out] type The message type.
 * @return  0
 */
CELIX_DFI_EXPORT int dynMessage_getMessageType(dyn_message_type *msg, dyn_type **type);

#ifdef __cplusplus
}
#endif

#endif
