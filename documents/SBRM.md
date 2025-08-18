---
title: Scope-Based Resource Management
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

# Scope-Based Resource Management (SBRM)

Apache Celix provides *auto pointers* and *auto values* as a lightweight form of scope-based resource management in C. 
These features are inspired by the RAII (Resource Acquisition Is Initialization) paradigm from C++, 
as well as ideas from [Scope-based resource management for the kernel](https://lwn.net/Articles/934679/).

Using the macros defined in `celix_cleanup.h` and related headers (`celix_stdio_cleanup.h`, `celix_stdlib_cleanup.h`), 
resources are automatically released when their associated variables go out of scope. This enables safer and more 
concise C code, especially when handling early returns or error paths.

## Defining Cleanup Functions

Before using auto cleanup, types must opt-in by defining a cleanup function.

- For pointer types:

```C
  CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_filter_t, celix_filter_destroy)
```

- For value types:

```C
CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_mutex_lock_guard_t, celixMutexLockGuard_deinit)
```

## Using Auto Pointers and Auto Values

### Auto Pointers

Use `celix_autoptr(type)` to declare a pointer that will be cleaned up automatically:

```C
void my_function(void) {
    celix_autoptr(celix_filter_t) filter = celix_filter_create("(foo=bar)");
    // use filter
} // filter is destroyed automatically
```

To transfer ownership and disable automatic cleanup, use:

```C
celix_filter_t* raw = celix_steal_ptr(filter);
```

### Auto Values

Use `celix_auto(type)` to declare a value-based resource with automatic cleanup:

```C
void my_function(void) {
    celix_auto(celix_mutex_lock_guard_t) lock = celixMutexLockGuard_init(&myMutex);
    // use guarded memory
} // lock guard is cleaned up automatically (myMutex is unlocked)
```

## Benefits

Celix's SBRM features allow for:

- Simplified cleanup logic
- RAII-style early returns
- Cleaner, more maintainable C code
