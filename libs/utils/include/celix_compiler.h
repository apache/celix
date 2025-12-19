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

#ifndef CELIX_CELIX_COMPILER_H
#define CELIX_CELIX_COMPILER_H
#ifdef __cplusplus
extern "C" {
#endif

/**
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html#index-cleanup-variable-attribute
 * clang: https://clang.llvm.org/docs/AttributeReference.html#cleanup
 */
#define CELIX_CLEANUP(func) __attribute__((cleanup(func)))

/**
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-unused-function-attribute
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Type-Attributes.html#index-unused-type-attribute
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html#index-unused-variable-attribute
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Label-Attributes.html#index-unused-label-attribute
 * clang: https://clang.llvm.org/docs/AttributeReference.html#maybe-unused-unused
 */
#define CELIX_UNUSED __attribute__((unused))

/* Indirect macros required for expanded argument pasting, eg. __LINE__. */
#define _CELIX_PASTE(a, b) a##b
#define CELIX_PASTE(a, b) _CELIX_PASTE(a, b)

#define CELIX_UNIQUE_ID(prefix) CELIX_PASTE(CELIX_PASTE(__UNIQUE_ID_, prefix), __COUNTER__)

/**
 * A wrapper around `__has_attribute()`, which tests for support for `__attribute__(())`s.
 *
 * See also:
 * clang: https://clang.llvm.org/docs/LanguageExtensions.html#has-attribute
 *   gcc: https://gcc.gnu.org/onlinedocs/cpp/_005f_005fhas_005fattribute.html
 */
#if defined(__has_attribute)
#define CELIX_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
#define CELIX_HAS_ATTRIBUTE(x) 0
#endif

/**
 * CELIX_OWNERSHIP_TAKES(type, idx)
 *
 * Expands to the compiler attribute `__attribute__((ownership_takes(type, idx)))`
 * when the compiler supports the `ownership_takes` attribute. This attribute
 * marks a function as a deallocating function.
 *
 * See also:
 * clang: https://clang.llvm.org/docs/AttributeReference.html#ownership-holds-ownership-returns-ownership-takes-clang-static-analyzer
 *
 * Parameters:
 *  - `type`: the type of the allocation: malloc, new, etc. to allow for catching mismatched deallocation bugs.
 *  - `idx`:  1-based index of the parameter that is being deallocated.
 *
 * Example:
 * ```C
 *  void CELIX_OWNERSHIP_TAKES(owner, 1) free_string(char* ptr);
 * ```
 */

#if CELIX_HAS_ATTRIBUTE(ownership_takes)
#define CELIX_OWNERSHIP_TAKES(type, idx) __attribute__((ownership_takes(type, idx)))
#else
#define CELIX_OWNERSHIP_TAKES(type, idx)
#endif

/**
 * CELIX_OWNERSHIP_RETURNS(type)
 *
 * Expands to the compiler attribute `__attribute__((ownership_returns(type, idx)))`
 * when the compiler supports the `ownership_returns` attribute. This attribute
 * marks a function as an allocating function.
 *
 * See also:
 * clang: https://clang.llvm.org/docs/AttributeReference.html#ownership-holds-ownership-returns-ownership-takes-clang-static-analyzer
 *
 * Parameters:
 *  - `type`: the type of the allocation: malloc, new, etc. to allow for catching mismatched deallocation bugs.
 *
 * Example:
 * ```C
 *  char CELIX_OWNERSHIP_RETURNS(owner)* allocate_string(void);
 * ```
 */
#if CELIX_HAS_ATTRIBUTE(ownership_returns)
#define CELIX_OWNERSHIP_RETURNS(type) __attribute__((ownership_returns(type)))
#else
#define CELIX_OWNERSHIP_RETURNS(type)
#endif

/**
 * CELIX_OWNERSHIP_HOLDS(type, idx)
 *
 * Expands to the compiler attribute `__attribute__((ownership_holds(type, idx)))`
 * when the compiler supports the `ownership_holds` attribute. This attribute
 * marks that a function takes over the ownership of a piece of memory and will free it
 * at some unspecified point in the future.
 *
 * See also:
 * clang: https://clang.llvm.org/docs/AttributeReference.html#ownership-holds-ownership-returns-ownership-takes-clang-static-analyzer
 *
 * Parameters:
 *  - `type`: the type of the allocation: malloc, new, etc. to allow for catching mismatched deallocation bugs.
 *  - `idx`:  1-based index of the parameter whose ownership will be taken over.
 *
 * Example:
 * ```C
 *  void CELIX_OWNERSHIP_HOLDS(owner, 1) hold_string(char* ptr);
 * ```
 */
#if CELIX_HAS_ATTRIBUTE(ownership_holds)
#define CELIX_OWNERSHIP_HOLDS(type, idx) __attribute__((ownership_holds(type, idx)))
#else
#define CELIX_OWNERSHIP_HOLDS(type, idx)
#endif

#ifdef __cplusplus
}
#endif
#endif // CELIX_CELIX_COMPILER_H
