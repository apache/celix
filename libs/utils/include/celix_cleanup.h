//  Licensed to the Apache Software Foundation (ASF) under one
//  or more contributor license agreements.  See the NOTICE file
//  distributed with this work for additional information
//  regarding copyright ownership.  The ASF licenses this file
//  to you under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance
//  with the License.  You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing,
//  software distributed under the License is distributed on an
//  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//  KIND, either express or implied.  See the License for the
//  specific language governing permissions and limitations
//  under the License.

#ifndef CELIX_CELIX_CLEANUP_H
#define CELIX_CELIX_CLEANUP_H
#ifdef __cplusplus
extern "C" {
#endif

#include "celix_compiler.h"

/* private */
#define _CELIX_AUTOPTR_FUNC_NAME(TypeName) celix_autoptr_cleanup_##TypeName
#define _CELIX_AUTOPTR_CLEAR_FUNC_NAME(TypeName) celix_autoptr_clear_##TypeName
#define _CELIX_AUTOPTR_TYPENAME(TypeName) TypeName##_autoptr
#define _CELIX_AUTO_FUNC_NAME(TypeName) celix_auto_cleanup_##TypeName
#define _CELIX_DEFINE_AUTOPTR_CLEANUP_FUNCS(TypeName, cleanup)                                                         \
    typedef TypeName* _CELIX_AUTOPTR_TYPENAME(TypeName);                                                               \
    static CELIX_UNUSED inline void _CELIX_AUTOPTR_CLEAR_FUNC_NAME(TypeName)(TypeName * _ptr) {         \
        if (_ptr)                                                                                                      \
            (cleanup)(_ptr);                                                                                           \
    }                                                                                                                  \
    static CELIX_UNUSED inline void _CELIX_AUTOPTR_FUNC_NAME(TypeName)(TypeName** _ptr) {               \
        _CELIX_AUTOPTR_CLEAR_FUNC_NAME(TypeName)(*_ptr);                                                               \
    }


/* API */
/**
 * @brief Defines the appropriate cleanup function for a pointer type.
 *
 * The function will not be called if the variable to be cleaned up contains NULL.
 * With this definition, it will be possible to use celix_autoptr() with @a TypeName.
 *
 * @param TypeName A type name to define a celix_autoptr() cleanup function for.
 * @param func The cleanup function.
 */
#define CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(TypeName, func) _CELIX_DEFINE_AUTOPTR_CLEANUP_FUNCS(TypeName, func)

/**
 * @brief Defines the appropriate cleanup function for a type.
 * With this definition, it will be possible to use celix_auto() with @a TypeName.
 * @param TypeName: A type name to define a celix_auto() cleanup function for.
 * @param func The clear function.
 */
#define CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(TypeName, func)                                                           \
    static CELIX_UNUSED inline void _CELIX_AUTO_FUNC_NAME(TypeName)(TypeName* _ptr) { (func)(_ptr); }

/**
 * @brief Defines the appropriate cleanup function for a type.
 *
 * With this definition, it will be possible to use celix_auto() with
 * @a TypeName.
 *
 * This function will be rarely used.  It is used with pointer-based
 * typedefs and non-pointer types where the value of the variable
 * represents a resource that must be freed.
 *
 * @a none specifies the "none" value for the type in question.  It is
 * probably something like NULL or `-1`.  If the variable is found to
 * contain this value then the free function will not be called.
 *
 * @param TypeName A type name to define a celix_auto() cleanup function for.
 * @param func The free function.
 * @param none The "none" value for the type.
 */
#define CELIX_DEFINE_AUTO_CLEANUP_FREE_FUNC(TypeName, func, none)                                                      \
    static CELIX_UNUSED inline void _CELIX_AUTO_FUNC_NAME(TypeName)(TypeName* _ptr)  {                  \
        if (*_ptr != none)                                                                                             \
            (func)(*_ptr);                                                                                             \
    }

/**
 * @brief Helper to declare a pointer variable with automatic cleanup.
 *
 * The variable is cleaned up in a way appropriate to its type when the
 * variable goes out of scope.  The type must support this.
 * The way to clean up the type must have been defined using the macro
 * CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC().
 *
 * This is meant to be used to declare pointers to types with cleanup
 * functions.  The type of the variable is a pointer to @a TypeName.  You
 * must not add your own `*`.
 */
#define celix_autoptr(TypeName) CELIX_UNUSED CELIX_CLEANUP(_CELIX_AUTOPTR_FUNC_NAME(TypeName)) _CELIX_AUTOPTR_TYPENAME(TypeName)

/**
 * @brief Helper to declare a variable with automatic cleanup.
 *
 * The variable is cleaned up in a way appropriate to its type when the
 * variable goes out of scope.  The type must support this.
 * The way to clean up the type must have been defined using one of the macros
 * CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC() or CELIX_DEFINE_AUTO_CLEANUP_FREE_FUNC().
 *
 * This is meant to be used with stack-allocated structures and
 * non-pointer types.  For the (more commonly used) pointer version, see
 * celix_autoptr().
 */
#define celix_auto(TypeName) CELIX_UNUSED CELIX_CLEANUP(_CELIX_AUTO_FUNC_NAME(TypeName)) TypeName

/**
 * @brief Transfer the ownership of the pointer from the
 * referenced variable to the "caller" of the macro.
 */
#ifdef __cplusplus
#define celix_steal_ptr(p) \
    ({ auto celix_ptr_ = (p); (p) = NULL; celix_ptr_; })
#else
#define celix_steal_ptr(p) \
    ({ __auto_type celix_ptr_ = (p); (p) = NULL; celix_ptr_; })
#endif


#ifdef __cplusplus
}
#endif
#endif // CELIX_CELIX_CLEANUP_H
