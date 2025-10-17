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

#ifdef __cplusplus
}
#endif
#endif // CELIX_CELIX_COMPILER_H
