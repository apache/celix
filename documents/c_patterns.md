---
title: Introduction
---

<!--
Licensed to the Apache Software Foundation (ASF) under one or more
contributor license agreements.  See the NOTICE file distributed with
this work for additional information regarding copyright ownership.
The ASF licenses this file to You under the Apache License, Version 2.0
(the "License"); you may not use this file except in compliance with
the License.  You may obtain a copy of the License at
   
    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
-->

# Apache Celix C Patterns

The core of Apache Celix is written in C, as C can serve as a common denominator for many languages. However, C lacks 
the concept of classes and objects, scope-based resource management - for concepts like RAII -, and other modern C++ 
features. To somewhat overcome this, Apache Celix employs several patterns.

It's important to note that ideally, all Apache Celix C code follows the patterns described in this section, but this 
isn't always the case. Particularly, older code may not always adhere to these patterns.

## Apache Celix C Objects

The first pattern is the Apache Celix C object pattern. This pattern is used to create, destroy, and manage objects in 
a C manner. A C object is implemented using an opaque pointer to a struct, which contains object details invisible to 
the object's user. The C object should provide C functions to create, destroy, and manipulate the object.

The naming scheme used for the object struct is `<celix_object_name>`, typically with a typedef to 
`<celix_object_name>_t`. For the object functions, the following naming scheme is 
used: `<celix_objectName>_<functionName>`. Note the camelCase for the object name and function name.

An Apache Celix C object should always have a constructor and a destructor. If memory allocation is involved, 
a `celix_<objectName>_create` function is used to create and return a new object, and a `celix_<objectName>_destroy` 
function is used to destroy the object and free the object's memory. Otherwise, use a `celix_<objectName>_init` function
with `celix_status_t` return value to initialize the object's provided memory and use a `celix_<objectName>_deinit` 
function to deinitialize the object. The `celix_<objectName>_deinit` function should not free the object's memory.

An Apache Celix C object can also have additional functions to access object information or to manipulate the object. 
If an object contains properties, it should provide a getter and setter function for each property.

## Apache Celix C Container Types

Apache Celix provides several container types: `celix_array_list`, `celix_properties`, `celix_string_hash_map`, 
and `celix_long_hash_map`. Although these containers are not type-safe, they offer additional functions to handle 
different element types. Refer to the header files for more information.

## Apache Celix C Scope-Based Resource Management

Apache Celix offers several macros to add support for scope-based resource management (SBRM) to existing types. 
These macros are inspired by [Scoped-based Resource Management for the Kernel](https://lwn.net/Articles/934838/).

The main macros used for SBRM are:
- `celix_autofree`: Automatically frees memory with `free` when the variable goes out of scope.
- `celix_auto`: Automatically calls a value-based cleanup function when the variable goes out of scope.
- `celix_autoptr`: Automatically calls a pointer-based cleanup function when the variable goes out of scope.
- `celix_steal_ptr`: Used to "steal" a pointer from a variable to prevent automatic cleanup when the variable goes 
                     out of scope.

These macros can be found in the Apache Celix utils headers `celix_cleanup.h` and `celix_stdlib_cleanup.h`.

In Apache Celix, C objects must opt into SBRM. This is done by using a `CELIX_DEFINE_AUTO` macro, which determines the 
expected C functions to clean up the object.

## Support for Resource Allocation Is Initialization (RAII)-like Structures

Based on the previously mentioned SBRM, Apache Celix also offers support for structures that resemble RAII. 
These can be used to guard locks, manage service registration, etc. These guards should follow the naming convention 
`celix_<obj_to_guard>_guard_t`. Support for RAII-like structures is facilitated by providing 
additional cleanup functions that work with either the `celix_auto` or `celix_autoptr` macros.

Examples include:
- `celix_mutex_lock_guard_t`
- `celix_service_registration_guard_t`

Special effort is made to ensure that these constructs do not require additional allocation and should provide minimal 
to no additional overhead.

## Polymorphism in Apache Celix

It's worth mentioning that the above-mentioned patterns and additions do not add support for polymorphism. 
Although this could be a welcome addition, Apache Celix primarily handles polymorphism through the use of services, 
both for itself and its users. Refer to the "Apache Celix Services" section for more information.
