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

#ifndef __DYN_INTERFACE_H_
#define __DYN_INTERFACE_H_

#include "dyn_type.h"
#include "dyn_function.h"
#include "celix_cleanup.h"
#include "celix_version.h"
#include "celix_dfi_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Description string
 *
 * Descriptor (interface) = HeaderSection AnnotationSection TypesSection MethodsSection
 *
 * HeaderSection=
 * ':header\n' [NameValue]*
 * ':annotations\n' [NameValue]*
 * ':types\n' [TypeIdValue]*
 * ':methods\n' [MethodIdValue]
 *
 */
typedef struct _dyn_interface_type dyn_interface_type;

TAILQ_HEAD(methods_head, method_entry);
struct method_entry {
    int index; ///< The index of the method in the interface
    char* id; ///< The signature of the method
    dyn_function_type* dynFunc; ///< The function type of the method

    TAILQ_ENTRY(method_entry) entries; 
};

/**
 * @brief Creates a dynamic interface type instance according to the given interface descriptor stream.
 *
 * The caller is the owner of the dyn_interface_type and the dyn_interface_type should be freed using dynInterface_destroy.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] descriptor The interface descriptor file stream.
 * @param[out] out The created dynamic interface type instance.
 * @return 0 if successful, 1 otherwise.
 */
CELIX_DFI_EXPORT int dynInterface_parse(FILE* descriptor, dyn_interface_type** out);

/**
 * @brief Destroys the given dynamic interface type instance.
 * @param[in] intf The dynamic interface type instance to destroy.
 */
CELIX_DFI_EXPORT void dynInterface_destroy(dyn_interface_type* intf);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(dyn_interface_type, dynInterface_destroy);

/**
 * @brief Gets the name of the given dynamic interface type instance.
 * The dynamic interface type instance is the owner of the returned string and the string should not be freed.
 * @note It always returns valid string.
 */
CELIX_DFI_EXPORT const char* dynInterface_getName(const dyn_interface_type* intf);

/**
 * @brief Gets the version of the given dynamic interface type instance.
 * The dynamic interface type instance is the owner of the version and the version should not be freed.
 * @note It always returns valid version.
 */
CELIX_DFI_EXPORT const celix_version_t* dynInterface_getVersion(const dyn_interface_type* intf);

/**
 * @brief Gets the version string of the given dynamic interface type instance.
 * The dynamic interface type instance is the owner of the version string and the version string should not be freed.
 * @note It always returns valid string.
 */
CELIX_DFI_EXPORT const char* dynInterface_getVersionString(const dyn_interface_type* intf);

/**
 * @brief Gets the value corresponding to the specified name, which comes from the header section of the given dynamic interface type instance.
 *
 * The dynamic interface type instance is the owner of the value and the value should not be freed.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] intf The dynamic interface type instance.
 * @param[in] name The name of the value to return.
 * @param[out] value The value corresponding to the specified name.
 * @return 0 if successful, 1 otherwise.
 */
CELIX_DFI_EXPORT int dynInterface_getHeaderEntry(const dyn_interface_type* intf, const char* name, const char** value);

/**
 * @brief Gets the value corresponding to the specified name, which comes from the annotation section of the given dynamic interface type instance.
 *
 * The dynamic interface type instance is the owner of the value and the value should not be freed.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] intf The dynamic interface type instance.
 * @param[in] name The name of the value to return.
 * @param[out] value The value corresponding to the specified name.
 * @return 0 if successful, 1 otherwise.
 */
CELIX_DFI_EXPORT int dynInterface_getAnnotationEntry(const dyn_interface_type* intf, const char* name, const char** value);

/**
 * @brief Gets the methods of the given dynamic interface type instance.
 * The dynamic interface type instance is the owner of the list and the list should not be freed.
 * @note It always returns valid methods.
 */
CELIX_DFI_EXPORT const struct methods_head* dynInterface_methods(const dyn_interface_type* intf);

/**
 * @brief Returns the number of methods for the given dynamic interface type instance.
 * @param[in] intf The dynamic interface type instance.
 * @return The number of methods for the given dynamic interface type instance.
 */
CELIX_DFI_EXPORT int dynInterface_nrOfMethods(const dyn_interface_type* intf);

/**
 * @brief Finds and returns the method_entry structure for a given method id in the dynamic interface type instance.
 * The dynamic interface type instance is the owner of the returned method_entry structure and it should not be freed.
 *
 * @param[in] intf The dynamic interface type instance.
 * @param[in] id The id of the method to find, which is currently the signature of the method.
 * @return The method_entry structure for the given method id, or NULL if no matching method is found.
 */
CELIX_DFI_EXPORT const struct method_entry* dynInterface_findMethod(const dyn_interface_type* intf, const char* id);


#ifdef __cplusplus
}
#endif

#endif
