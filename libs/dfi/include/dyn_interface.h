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

#include "dyn_common.h"
#include "dyn_type.h"
#include "dyn_function.h"
#include "dfi_log_util.h"
#include "celix_cleanup.h"
#include "celix_version.h"
#include "celix_dfi_export.h"

#ifdef __cplusplus
extern "C" {
#endif

DFI_SETUP_LOG_HEADER(dynInterface);

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
    int index;
    char *id;
    char *name;
    dyn_function_type *dynFunc;

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
CELIX_DFI_EXPORT int dynInterface_parse(FILE *descriptor, dyn_interface_type **out);

/**
 * @brief Destroys the given dynamic interface type instance.
 * @param[in] intf The dynamic interface type instance to destroy.
 */
CELIX_DFI_EXPORT void dynInterface_destroy(dyn_interface_type *intf);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(dyn_interface_type, dynInterface_destroy);

/**
 * @brief Gets the name of the given dynamic interface type instance.
 *
 * The dynamic interface type instance is the owner of the returned string and the string should not be freed.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] intf The dynamic interface type instance.
 * @param[out] name The name of the dynamic interface type instance.
 * @return 0 if successful, 1 otherwise.
 */
CELIX_DFI_EXPORT int dynInterface_getName(dyn_interface_type *intf, char **name);

/**
 * @brief Gets the version of the given dynamic interface type instance.
 *
 * The dynamic interface type instance is the owner of the version and the version should not be freed.
 *
 * @param[in] intf The dynamic interface type instance.
 * @param[out] version The version of the dynamic interface type instance.
 * @return 0 if successful, 1 otherwise.
 */
CELIX_DFI_EXPORT int dynInterface_getVersion(dyn_interface_type *intf, celix_version_t** version);

/**
 * @brief Gets the version string of the given dynamic interface type instance.
 *
 * The dynamic interface type instance is the owner of the version string and the version string should not be freed.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] intf The dynamic interface type instance.
 * @param[out] version The version string of the dynamic interface type instance.
 * @return 0 if successful, 1 otherwise.
 */
CELIX_DFI_EXPORT int dynInterface_getVersionString(dyn_interface_type *intf, char **version);

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
CELIX_DFI_EXPORT int dynInterface_getHeaderEntry(dyn_interface_type *intf, const char *name, char **value);

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
CELIX_DFI_EXPORT int dynInterface_getAnnotationEntry(dyn_interface_type *intf, const char *name, char **value);

/**
 * @brief Gets the methods of the given dynamic interface type instance.
 *
 * The dynamic interface type instance is the owner of the list and the list should not be freed.
 *
 * @param[in] intf The dynamic interface type instance.
 * @param[out] list The method list of the dynamic interface type instance.
 * @return 0.
 */
CELIX_DFI_EXPORT int dynInterface_methods(dyn_interface_type *intf, struct methods_head **list);

/**
 * @brief Returns the number of methods for the given dynamic interface type instance.
 * @param[in] intf The dynamic interface type instance.
 * @return The number of methods for the given dynamic interface type instance.
 */
CELIX_DFI_EXPORT int dynInterface_nrOfMethods(dyn_interface_type *intf);


#ifdef __cplusplus
}
#endif

#endif
