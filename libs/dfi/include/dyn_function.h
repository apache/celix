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

#ifndef __DYN_FUNCTION_H_
#define __DYN_FUNCTION_H_

#include "dyn_type.h"
#include "celix_cleanup.h"
#include "celix_dfi_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Uses the following schema
 * (Name)([Type]*)Type
 *
 * Dyn function argument meta (am) as meta info, with the following possible values
 * am=handle #void pointer for the handle
 * am=pre #output pointer with memory pre-allocated, it should be pointer to trivial types, check `dynType_isTrivial` for more info.
 * am=out #output pointer, it should be pointer to text or double pointer to serializable types
 *
 * Without meta info the argument is considered to be a standard argument, which can be of any serializable type.
 *
 * text argument (t) can also be annotated to be considered const string.
 * Normally a text argument will be handled as char*, meaning that the callee is expected to take of ownership.
 * If a const=true annotation is used the text argument will be handled as a const char*, meaning that the caller
 * keeps ownership of the string.
 */

typedef struct _dyn_function_type dyn_function_type;

enum dyn_function_argument_meta {
    DYN_FUNCTION_ARGUMENT_META__STD = 0,
    DYN_FUNCTION_ARGUMENT_META__HANDLE = 1,
    DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT = 2,
    DYN_FUNCTION_ARGUMENT_META__OUTPUT = 3
};

typedef struct _dyn_function_argument_type dyn_function_argument_type;
TAILQ_HEAD(dyn_function_arguments_head,_dyn_function_argument_type);
struct _dyn_function_argument_type {
    int index;
    enum dyn_function_argument_meta argumentMeta;
    dyn_type* type;
    TAILQ_ENTRY(_dyn_function_argument_type) entries;
};

/**
 * @brief Creates a dyn_function_type according to the given function descriptor stream.
 *
 * The caller is the owner of the dynFunc and the dynFunc should be freed using dynFunction_destroy.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] descriptorStream The stream containing the function descriptor.
 * @param[in] refTypes A list if reference-able dyn types.
 * @param[out] dynFunc The created dynamic type instance for function.
 * @return 0 If successful
 * @retval 1 If there is not enough memory to create dyn_function_type.
 * @retval 2 Errors other than out-of-memory.
 */
CELIX_DFI_EXPORT int dynFunction_parse(FILE* descriptorStream, struct types_head* refTypes, dyn_function_type** dynFunc);

/**
 * @brief Creates a dyn_function_type according to the given function descriptor string.
 *
 * The caller is the owner of the dynFunc and the dynFunc should be freed using dynFunction_destroy.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] descriptor The string containing the function descriptor.
 * @param[in] refTypes A list if reference-able dyn types.
 * @param[out] dynFunc The created dynamic type instance for function.
 * @return 0 If successful
 * @retval 1 If there is not enough memory to create dyn_function_type.
 * @retval 2 Errors other than out-of-memory.
 */
CELIX_DFI_EXPORT int dynFunction_parseWithStr(const char* descriptor, struct types_head* refTypes, dyn_function_type** dynFunc);

/**
 * @brief Returns the number of arguments of the given dynamic function type instance.
 * @param[in] dynFunc The dynamic type instance for function.
 * @return The number of arguments.
 */
CELIX_DFI_EXPORT int dynFunction_nrOfArguments(const dyn_function_type* dynFunc);

/**
 * @brief Returns the argument type for the given argument index.
 * @param[in] dynFunc The dynamic type instance for function.
 * @param[in] argumentNr The argument index.
 * @return The argument type.
 */
CELIX_DFI_EXPORT const dyn_type* dynFunction_argumentTypeForIndex(const dyn_function_type* dynFunc, int argumentNr);

/**
 * @brief Returns the argument meta for the given argument index.
 * @param[in] dynFunc The dynamic type instance for function.
 * @param[in] argumentNr The argument index.
 * @return The argument meta.
 */
CELIX_DFI_EXPORT enum dyn_function_argument_meta dynFunction_argumentMetaForIndex(const dyn_function_type* dynFunc, int argumentNr);

/**
 * @brief Returns the argument list for the given dynamic function type instance.
 * @note It always returns valid list.
 */
CELIX_DFI_EXPORT const struct dyn_function_arguments_head* dynFunction_arguments(const dyn_function_type* dynFunc);

/**
 * @brief Returns the return value type for the given dynamic function type instance.
 * @param[in] dynFunc The dynamic type instance for function.
 * @return The return value type.
 */
CELIX_DFI_EXPORT const dyn_type* dynFunction_returnType(const dyn_function_type* dynFunction);

/**
 * @brief Destroys the given dynamic function type instance.
 * @param[in] dynFunc The dynamic type instance for function.
 */
CELIX_DFI_EXPORT void dynFunction_destroy(dyn_function_type* dynFunc);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(dyn_function_type, dynFunction_destroy);

/**
 * @brief Calls the given dynamic type function.
 * @param[in] dynFunc The dynamic type instance for function.
 * @param[in] fn The function pointer to call.
 * @param[in] returnValue The return value pointer.
 * @param[in] argValues The argument values.
 * @return 0
 */
CELIX_DFI_EXPORT int dynFunction_call(const dyn_function_type* dynFunc, void(*fn)(void), void* returnValue, void** argValues);

/**
 * @brief Creates a closure for the given dynamic function type instance.
 * @param[in] func The dynamic type instance for function.
 * @param[in] bind The bind function to use for the closure.
 * @param[in] userData The user data to use for the closure.
 * @param[out] fn The function pointer to call.
 * @return 0 If successful
 * @retval 1 If there is not enough memory to create the closure.
 * @retval 2 Errors other than out-of-memory.
 */
CELIX_DFI_EXPORT int dynFunction_createClosure(dyn_function_type* func, void (*bind)(void*, void**, void*), void* userData, void(**fn)(void));

/**
 * @brief Returns the function pointer for the given dynamic function type instance.
 * @param[in] func The dynamic type instance for function.
 * @param[out] fn The function pointer.
 * @return 0 If successful, 1 if the dynamic function type instance has no function pointer.
 */
CELIX_DFI_EXPORT int dynFunction_getFnPointer(const dyn_function_type* func, void (**fn)(void));

/**
 * Returns whether the function has a return type.
 * Will return false if return is void.
 */
CELIX_DFI_EXPORT bool dynFunction_hasReturn(const dyn_function_type* dynFunction);

/**
 * @brief Returns the name of the given dynamic function type instance.
 */
CELIX_DFI_EXPORT const char* dynFunction_getName(const dyn_function_type* func);

#ifdef __cplusplus
}
#endif

#endif
