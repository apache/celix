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

#include <stdlib.h>

/* private */
#define _CELIX_AUTOPTR_FUNC_NAME(TypeName) celix_autoptr_cleanup_##TypeName
#define _CELIX_AUTOPTR_CLEAR_FUNC_NAME(TypeName) celix_autoptr_clear_##TypeName
#define _CELIX_AUTOPTR_TYPENAME(TypeName) TypeName##_autoptr
#define _CELIX_AUTO_FUNC_NAME(TypeName) celix_auto_cleanup_##TypeName
#define _CELIX_CLEANUP(func) __attribute__((cleanup(func)))
#define _CELIX_DEFINE_AUTOPTR_CLEANUP_FUNCS(TypeName, cleanup)                                                         \
    typedef TypeName* _CELIX_AUTOPTR_TYPENAME(TypeName);                                                               \
    static __attribute__((__unused__)) inline void _CELIX_AUTOPTR_CLEAR_FUNC_NAME(TypeName)(TypeName * _ptr) {         \
        if (_ptr)                                                                                                      \
            (cleanup)(_ptr);                                                                                \
    }                                                                                                                  \
    static __attribute__((__unused__)) inline void _CELIX_AUTOPTR_FUNC_NAME(TypeName)(TypeName** _ptr) {               \
        _CELIX_AUTOPTR_CLEAR_FUNC_NAME(TypeName)(*_ptr);                                                               \
    }


/* API */
#define CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(TypeName, func) _CELIX_DEFINE_AUTOPTR_CLEANUP_FUNCS(TypeName, func)
#define CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(TypeName, func)                                                           \
    static __attribute__((__unused__)) inline void _CELIX_AUTO_FUNC_NAME(TypeName)(TypeName* _ptr) { (func)(_ptr); }
#define CELIX_DEFINE_AUTO_CLEANUP_FREE_FUNC(TypeName, func, none)                                                      \
    static __attribute__((__unused__)) inline void _CELIX_AUTO_FUNC_NAME(TypeName)(TypeName* _ptr)  {                  \
        if (*_ptr != none)                                                                                             \
            (func)(*_ptr);                                                                                             \
    }

#define celix_autoptr(TypeName) _CELIX_CLEANUP(_CELIX_AUTOPTR_FUNC_NAME(TypeName)) _CELIX_AUTOPTR_TYPENAME(TypeName)
#define celix_auto(TypeName) _CELIX_CLEANUP(_CELIX_AUTO_FUNC_NAME(TypeName)) TypeName

static inline void celix_autoptr_cleanup_generic_free(void* p) {
    void** pp = (void**)p;
    free(*pp);
}

#define celix_autofree _CELIX_CLEANUP(celix_autoptr_cleanup_generic_free)

#ifdef __cplusplus
#define celix_steal_ptr(p) \
    ({ auto __ptr = (p); (p) = NULL; __ptr; })
#else
#define celix_steal_ptr(p) \
    ({ __auto_type __ptr = (p); (p) = NULL; __ptr; })
#endif


#ifdef __cplusplus
}
#endif
#endif // CELIX_CELIX_CLEANUP_H
