---
title: Apache Celix Coding Conventions
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

# Apache Celix Coding Conventions
Adhering to consistent and meaningful coding conventions is crucial for maintaining readable and maintainable code.
This document outlines the recommended coding conventions for Apache Celix development, including naming conventions,
formatting, comments, control structures, functions and error handling.

Note that not all existing code adheres to these conventions.
New code should adhere to these conventions, and when possible, existing code should be updated to adhere to these
conventions.

## Naming Conventions

### C/C++ Variables

- Use `camelCase` for variable names.
- Use descriptive names for variables.
- Use `celix_` prefix or `celix::` (sub)namespace for global variables.
- Asterisks `*` and ampersands `&` should be placed on the variable type name.

### C Structures

- Use `snake_case` for structure names.
- Add a typedef for the structure.
- Use a `_t` postfix for structure typedef.
- Use `celix_` prefix for structure names.
- For C objects, use typedef of an opaque struct. E.g. `typedef struct celix_<obj> celix_<obj>_t;`
  - This way the implementation details can be hidden from the user.

### C Functions

- Use descriptive names for functions.
- Use a `celix_` prefix.
- Use a `_<obj>_` camelCase infix for the object/module name.
- Use a postfix `camelCase` for the function name.
- Asterisks `*` should be placed on the variable type name.
- Use verb as function names when a function has a side effect.
- Use nouns or getter/setter as function names when a function does not have a side effect.
- Use getters/setters naming convention for functions which get/set a value:
  - `celix_<obj>_is<Value>` and `celix_<obj>_set<Value>` for boolean values
  - `celix_<obj>_get<Value>` and `celix_<obj>_set<Value>` for other values
- For C objects:
  - Use a (opaque) object pointer as the first argument of the function.
  - Ensure that object can be created using a `celix_<obj>_create` function and destroyed using 
    a `celix_<obj>_destroy` function.
  - The `celix_<obj>_create` function should return a pointer to the object.
  - The `celix_<obj>_destroy` function should return a `void` and should be able to handle a NULL pointer.
    - By being able to handle a NULL pointer, the `celix_<obj>_destroy` function can be more easily used in
      error handling code.

Examples:
- `long celix_bundleContext_installBundle(celix_bundle_context_t* ctx, const char* bundleUrl, bool autoStart)`
- `bool celix_utils_stringEquals(const char* a, const char* b)`
- `celix_status_t celix_utils_createDirectory(const char* path, bool failIfPresent, const char** errorOut)`

### C Constants

- Use `SNAKE_CASE` for constant names.
- Use a `CELIX_` prefix for constant names.
- Use `#define` for constants.

### C Enums

- Use `snake_case` for enum type names.
- Use a `celix_` prefix for enum type names.
- Use `SNAKE_CASE` for enum value names.
- Use a `CELIX_` prefix for enum value names.
- Add a typedef - with a `_e` postfix - for the enum

Example:
```c
typedef enum celix_hash_map_key_type {
    CELIX_HASH_MAP_STRING_KEY,
    CELIX_HASH_MAP_LONG_KEY
} celix_hash_map_key_type_e;
```

### Macros

- Use all caps `SNAKE_CASE` for macro names.
- Use a `CELIX_` prefix for macro names.

### C files and directories

- Use `snake_case` for file names.
- Name header files with a `.h` extension and source files with a `.c` extension.
- Organize files in directories according to their purpose.
  - Public headers files in a `include`, `api` or `spi` directory.
  - Private header files in a `private` and `src` directory.
  - Source files in a `src` directory.
- Google test files should be placed in a `gtest` directory with its own `CMakeLists.txt` file and `src` directory.
- Use `celix_` prefix for header file names.
- Use a header guard.
- Use a C++ "extern C" block in headers file to ensure C headers are usable in C++.

### C Libraries

- Target names should be `snake_case`.
- There should be `celix::` prefixed aliases for the library.
- C Shared libraries should configure an output name with a `celix_` prefix.

### C Services

- Service headers should be made available through a CMake INTERFACE header-only api/spi library (i.e. `celix::shell_api`)
- C service should be C struct, where the first member is the service handle (`void* handle;`) and the rest of the members are
  function pointers.
- The first argument of the service functions should be the service handle.
- If memory allocation is needed or another error can occur in a service function, ensure that the return value can
  be used to check for errors. This can be done by:
  - Returning a `celix_status_t` and if needed using an out parameter.
  - Returning a NULL pointer if the function returns a pointer type.
  - Returning a boolean value, where `true` indicates success and `false` indicates failure.
- In the same header as the C service struct, there should be defines for the service name and version.
- The service name macro  should be all caps `SNAKE_CASE`, prefixed with `CELIX_` and postfixed with `_NAME`.
- The service version macro should be all caps `SNAKE_CASE`, prefixed with `CELIX_` and postfixed with `_VERSION`.
- The value of the service name macro should be the service struct (so without a `_t` postfix
- The value of the service version macro should be the version of the service.

Example:
```c
//celix_foo.h
#include "celix_errno.h"

#define CELIX_FOO_NAME "celix_foo"
#define CELIX_FOO_VERSION 1.0.0

typedef struct celix_foo {
    void* handle;
    celix_status_t (*doFoo)(void* handle, char** outMsg);
} celix_foo_t;
```

### C Bundles

- Use `snake_case` for C bundle target names.
- Do _not_ use a `celix_` prefix for C bundle target names.
- Use `celix::` prefixed aliases for C bundle targets.
- Use `snake_case` for C bundle symbolic names.
- Configure at least SYMBOLIC_NAME, NAME, FILENAME, VERSION and GROUP for C bundle targets.
- Use `apache_celix_` prefix for C bundle symbolic names.
- Use `Apache Celix ` prefix for C bundle names.
- Use a `celix_` prefix for C bundle filenames.
- Use a group name starting with `celix/` for C bundle groups.

Examples:
```cmake
add_celix_bundle(my_bundle
    SOURCES src/my_bundle.c
    SYMBOLIC_NAME "apache_celix_my_bundle"
    NAME "Apache Celix My Bundle"
    FILENAME "celix_my_bundle"
    VERSION "1.0.0"
    GROUP "celix/my_bundle_group"
)
add_library(celix::my_bundle ALIAS my_bundle)
```

### C++ Namespaces

- Use `snake_case` for namespace names.
- All namespaces should be part of the `celix` namespace.
- Aim for a max of 3 levels of namespaces.
- Use a namespace ending with `detail` for implementation details.

### C++ Classes

- Use `CamelCase` (starting with a capital) for class names.
- Use descriptive names for classes.
- Classes should be part of a `celix::` namespace or sub `celix::` namespace.

### C++ Functions

- Use `camelCase` for function names.
- If a function is not part of a class/struct, it should be part of a `celix::` namespace or sub `celix::` namespace.
- Asterisks `*` and ampersands `&` should be placed on the variable type name.
- Use verb as function names when a function has a side effect.
- Use nouns or getter/setter as function names when a function does not have a side effect.
- Use getters/setters naming convention for functions which get/set a value.


### C++ Constants

- Use `SNAKE_CASE` for constants.
- Use constexpr for constants.
- Place constants in a `celix::` namespace or sub `celix::` namespace.

example:
```c++
namespace celix {
    constexpr long FRAMEWORK_BUNDLE_ID = 0;
    constexpr const char* const SERVICE_ID = "service.id";
}
```

### C++ Enums

- Use `CamelCase` (starting with a capital) for enum types names.
- Use `enum class` instead of `enum` and if possible use `std::int8_t` as base type.
- Use `SNAKE_CASE` for enum values without a celix/class prefix. Note that for enum values no prefix is required 
  because enum class values are scoped.

Example:
```c++
namespace celix {
    enum class ServiceRegistrationState {
        REGISTERING,
        REGISTERED,
        UNREGISTERING,
        UNREGISTERED
    };
}
```

### C++ files and directories

- Use `CamelCase` (starting with a capital) for file names.
- Name header files with a `.h` extension and source files with a `.cc` extension.
- Place header files in a directory based on the namespace (e.g. `celix/Bundle.h`, `celix/dm/Component.h`).
- Organize files in directories according to their purpose.
  - Public headers files in a `include`, `api` or `spi` directory.
  - Private header files in a `private` and `src` directory.
  - Source files in a `src` directory.
- Use a `#pragma once` header guard.

### C++ Libraries

- Target name should be `CamelCase` (starting with a capital).
- There should be `celix::` prefixed aliases for the library.
- C++ Libraries should support C++14.
  - Exception are `celix::Promises` and `celix::PushStreams` which requires C++17.
- The Apache Celix framework library (`Celix::framework`) and the Apache Celix utils library (`Celix::utils`) can only
  use header-only C++ files. This ensure that the framework and utils library can be used in C only projects and do
  not introduce a C++ ABI.
- For other libraries, header-only C++ libraries are preferred but not required.
- Header-only C++ libraries do not need an export header and do not need to configure symbol visibility.
- C++ shared libraries (lib with C++ sources), should configure an output name with a `celix_` prefix.
- C++ shared libraries (lib with C++ sources), should use an export header and configure symbol visibility.
  - See the C Libraries section for more information.

### C++ Services

- Use `CamelCase` (starting with a capital) for service names.
- Add a 'I' prefix to the service interface name.
- Place service classes in a `celix::` namespace or sub `celix::` namespace.
- Add a `static constexpr const char* const NAME` to the service class, for the service name.
- Add a `static constexpr const char* const VERSION` to the service class, for the service version.

### C++ Bundles

- Use `CamelCase` for C++ bundle target names.
- Do _not_ use a `Celix` prefix for C++ bundle target names.
- Use `celix::` prefixed aliases for C++ bundle targets.
- Use `CamelCase` for C++ bundle symbolic names.
- Configure at least SYMBOLIC_NAME, NAME, FILENAME, VERSION and GROUP for C++ bundle targets.
- Use `Apache_Celix_` prefix for C++ bundle symbolic names.
- Use `Apache Celix ` prefix for C++ bundle names.
- Use a `celix_` prefix for C++ bundle filenames.
- Use a group name starting with `celix/` for C++ bundle groups.

Examples:
```cmake
add_celix_bundle(MyBundle
    SOURCES src/MyBundle.cc
    SYMBOLIC_NAME "Apache_Celix_MyBundle"
    NAME "Apache Celix My Bundle"
    FILENAME "celix_MyBundle"
    VERSION "1.0.0"
    GROUP "celix/MyBundleGroup"
)
add_library(celix::MyBundle ALIAS MyBundle)
```

### Unit Tests Naming

- The test fixture should have a`TestSuite` postfix.
- The source file should be named after the test fixture name and use a `.cc` extension.
- Testcase names should use `CamelCase` (starting with a capital) and have a `Test` postfix.
- When using error injection (one of the `error_injector` libraries) a separate test suite should be used.
  - A `ErrorInjectionTestSuite` postfix should be used for the test fixture.
  - The error injection setup should be reset on the `TearDown` function or destructor of the test fixture.

## Comments and Documentation

- Use Doxygen documentation, except for inline comments.
- Write comments that explain the purpose of the code, focusing on the "why" rather than the "how".
- Apply doxygen documentation to all public API's.
- Use the javadoc style for doxygen documentation.
- Use `@` instead of `\` for doxygen commands.
- Start with a `@brief` command and a short description.
- For `@param` commands also provide in, out, or in/out information.
- For `@return` commands also provide a description of the return value.
- If a function can return multiple error codes, use a errors section (`@section errors_section Errors`) to document the 
  possible errors. Use `man 2 write` as an example for a good errors section.

  
## Formatting and Indentation

- Use spaces for indentation and use 4 spaces per indentation level.
- Keep line lengths under 120 characters, if possible, to enhance readability.
- Place opening braces on the same line as the control statement or function definition,  
  and closing braces on a new line aligned with the control statement or function definition.
- Use a single space before and after operators and around assignment statements.
- Add a space after control keywords (`if`, `for`, `while`, etc.) that are followed by a parenthesis.
- Always use braces ({ }) for control structures, even for single-statement blocks, to prevent errors.
- Add a space after control keywords (`else`, `do`, etc) that are followed by a brace.
- Do not add a space after the function name and the opening parenthesis.
- For new files apply clang-format using the project .clang-format file.
  - Note that this can be done using a plugin for your IDE or by running `clang-format -i <file>`.

## Control Structures

- Use `if`, `else if`, and `else` statements to handle multiple conditions.
- Use `switch` statements for multiple conditions with a default case.
- Use `while` statements for loops that may not execute.
- Use `do`/`while` statements for loops that must execute at least once.
- Use `for` statements for loops with a known number of iterations.
- Avoid using `goto` for error handling. Prefer early returns and automatic cleanup using celix auto pointers.
- To prevent deeply nested control structures, the `CELIX_DO_IF` macro can also be used.

## Functions and Methods

- Limit functions to a single responsibility or purpose, following the Single Responsibility Principle (SRP).
- Keep functions short and focused, aiming for a length of fewer than 50 lines.
- Ensure const correctness.
- For C functions with a lot of different parameters, consider using an options struct.
  - An options struct combined with a EMPTY_OPTIONS macro can be used to provide default values and a such
    options struct can be updated backwards compatible.
  - An options struct ensure that a lot of parameters can be configured, but also direct set on creation.
- For C++ functions with a lot of different parameters, consider using a builder pattern.
  - A builder pattern can be updated backwards compatible.
  - A builder pattern ensure that a lot of parameters can be configured, but also direct set on construction.

## Error Handling and Logging

- For C++, throw an exception when an error occurs and use RAII to ensure that resources are freed.
- For C, if memory allocation is needed or another error can occur, ensure that a function returns a value 
  than can be used to check for errors. This can be done by:
  - Returning a `celix_status_t` and if needed using an out parameter.
  - Returning a NULL pointer if the function returns a pointer.
  - Returning a boolean value, where `true` indicates success and `false` indicates failure.
- Use consistent error handling techniques, such as returning error codes or using designated error handling functions.
- Log errors, warnings, and other important events using the Apache Celix log helper functions or - for libraries - 
  the `celix_err` functionality. 
- Always check for errors and log them.
- Error handling should free resources in the reverse order of their allocation/creation.
- Ensure error handling is correct, using test suite with error injection.
 - Prefer early returns together with celix auto pointers to ensure cleanup without using `goto`.

For log levels use the following guidelines:
- trace: Use this level for very detailed that you would only want to have while diagnosing problems. 
- debug: This level should be used for information that might be helpful in diagnosing problems or understanding 
  what's going on, but is too verbose to be enabled by default. 
- info: Use this level for general operational messages that aren't tied to any specific problem or error condition. 
  They provide insight into the normal behavior of the system. Examples include startup/shutdown messages, 
  configuration assumptions, etc.
- warning: Use this level to report an issue from which the system can recover, but which might indicate a potential 
  problem.
- error: This level should be used to report issues that need immediate attention and might prevent the system from 
  functioning correctly. These are problems that are unexpected and affect functionality, but not so severe that the 
  process needs to stop. Examples include runtime errors, inability to connect to a service, etc.
- fatal: Use this level to report severe errors that prevent the program from continuing to run. 
  After logging a fatal error, the program will typically terminate.

Example of error handling and logging using auto pointers:
```c
typedef struct celix_foo {
    celix_thread_mutex_t mutex;
    bool mutexInitialized;
    celix_array_list_t* list;
    celix_long_hash_map_t* map;
} celix_foo_t;

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_foo_t, celix_foo_destroy)

celix_foo_t* celix_foo_create(celix_log_helper_t* logHelper) {
    celix_autoptr(celix_foo_t) foo = calloc(1, sizeof(*foo));
    if (!foo) {
        celix_logHelper_log(logHelper, CELIX_LOG_LEVEL_ERROR,
                "Error creating foo, out of memory");
        return NULL;
    }

    if (celixThreadMutex_create(&foo->mutex, NULL) == CELIX_SUCCESS) {
        foo->mutexInitialized = true;
    } else {
        celix_logHelper_log(logHelper, CELIX_LOG_LEVEL_ERROR,
                "Error creating mutex");
        return NULL; //foo cleaned up automatically (celix_autoptr(celix_foo_t) will call celix_foo_destroy)
    }

    foo->list = celix_arrayList_create();
    foo->map = celix_longHashMap_create();
    if (!foo->list || !foo->map) {
        celix_logHelper_log(logHelper, CELIX_LOG_LEVEL_ERROR,
                "Error creating foo, out of memory");
        return NULL; //foo cleaned up automatically (celix_autoptr(celix_foo_t) will call celix_foo_destroy)
    }

    return celix_steal_ptr(foo);
}

void celix_foo_destroy(celix_foo_t* foo) {
    if (foo) {
        //note reverse order of creation
        if (foo->mutexInitialized) {
            celixThreadMutex_destroy(&foo->mutex);
        }
        celix_arrayList_destroy(foo->list);
        celix_longHashMap_destroy(foo->map);
        free(foo);
    }
}
```

## Error Injection

- Use the Apache Celix error_injector libraries to inject errors in unit tests in a controlled way.
- Create a separate test suite for error injection tests and place them under a `EI_TESTS` cmake condition.
- Reset error injection setup on the `TearDown` function or destructor of the test fixture.
- If an - internal or external - function is missing error injection support, add it to the error_injector library.
  - Try to create small error injector libraries for specific functionality.

## Unit Test Approach

- Use the Google Test framework for unit tests.
- Use the Google Mock framework for mocking.
- Use the Apache Celix error_injector libraries to inject errors in unit tests in a controlled way.
- Test bundles by installing them in a programmatically created framework.
- Test bundles by using their provided services and used services.
- In most cases, libraries can be tested using a white box approach and bundles can be tested using a black box approach.
- For libraries that are tested with the Apache Celix error_injector libraries or require access to private/hidden 
  functions (white-box testing), a separate "code under test" static library should be created. 
  This library should not hide its symbols and should have a `_cut` postfix.

 
```cmake
set(MY_LIB_SOURCES ...)
set(MY_LIB_PUBLIC_LIBS ...)
set(MY_LIB_PRIVATE_LIBS ...)
add_library(my_lib SHARED ${MY_LIB_SOURCES})
target_link_libraries(my_lib PUBLIC ${MY_LIB_PUBLIC_LIBS} PRIVATE ${MY_LIB_PRIVATE_LIBS})
celix_target_hide_symbols(my_lib)

if (ENABLE_TESTING)
    add_library(my_lib_cut STATIC ${MY_LIB_SOURCES})
    target_link_libraries(my_lib_cut PUBLIC ${MY_LIB_PUBLIC_LIBS} ${MY_LIB_PRIVATE_LIBS})
    target_include_directories(my_lib_cut PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_BINARY_DIR}/celix/gen/includes/my_lib
    )
endif ()
```

## Supported C and C++ Standards

- C libraries should support C99.
- C++ libraries should support C++14.
  - Exception are `celix::Promises` and `celix::PushStreams` which requires C++17.
- C++ support for `celix::framework` and `celix::utils` must be header-only. 
- Unit test code can be written in C++17.

## Library target properties

For C and C++ shared libraries, the following target properties should be set:
 - `VERSION` should be set to the library version.
 - `SOVERSION` should be set to the library major version.
 - `OUTPUT_NAME` should be set to the library name and should contain a `celix_` prefix.

```cmake
add_library(my_lib SHARED
    src/my_lib.c)
set_target_properties(my_lib 
    PROPERTIES
        VERSION 1.0.0
        SOVERSION 1
        OUTPUT_NAME celix_my_lib)
```

For C and C++ static libraries, the following target properties should be set:
 - `POSITION_INDEPENDENT_CODE` should be set to `ON` for static libraries.
 - `OUTPUT_NAME` should be set to the library name and should contain a `celix_` prefix.

```cmake
add_library(my_lib STATIC
    src/my_lib.c)
set_target_properties(my_lib
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        OUTPUT_NAME celix_my_lib)
```

## Symbol Visibility

- Header-only (INTERFACE) libraries should not configure symbol visibility.
- Shared and static libraries should configure symbol visibility.
  - Static library meant to be linked as PRIVATE should hide symbols.
- Bundles should configure symbol visibility (this is done by default).

### Configuring Symbol Visibility for C/C++ Libraries

For Apache Celix shared libraries, symbol visibility should be configured using the CMake target 
properties `C_VISIBILITY_PRESET`, `CXX_VISIBILITY_PRESET` and `VISIBILITY_INLINES_HIDDEN` and a generated export 
header. 

The `C_VISIBILITY_PRESET` and `CXX_VISIBILITY_PRESET` target properties can be used to configure the default visibility 
of symbols in C and C++ code. The `VISIBILITY_INLINES_HIDDEN` property can be used to configure the visibility of 
inline functions. The `VISIBILITY_INLINES_HIDDEN` property is only supported for C++ code.

The default visibility should be configured to hidden and symbols should be explicitly exported using the export
marcos from a generated export header. The export header can be generated using the CMake function 
`generate_export_header`. Every library should have its own export header. 

For shared libraries, this can be done using the following CMake code:

```cmake
add_library(my_lib SHARED
        src/my_lib.c)
set_target_properties(my_lib PROPERTIES
        C_VISIBILITY_PRESET hidden
        #For C++ shared libraries also configure CXX_VISIBILITY_PRESET
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        OUTPUT_NAME celix_my_lib)
target_include_directories(my_lib
      PUBLIC
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/celix/gen/includes/my_lib>
      PRIVATE
        src)

#generate export header
generate_export_header(my_lib
        BASE_NAME "CELIX_MY_LIB"
        EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/celix/gen/includes/my_lib/celix_my_lib_export.h")

#install
install(TARGETS my_lib EXPORT celix LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix_my_lib)
install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix_my_lib)
install(DIRECTORY ${CMAKE_BINARY_DIR}/celix/gen/includes/my_lib/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix_my_lib)
```

### Configuring Symbol Visibility for C/C++ Bundles

For bundle, symbol visibility will default be configured to hidden. This can be default by providing
the `DO_NOT_CONFIGURE_SYMBOL_VISIBILITY` option to the CMake `add_celix_bundle` function.

If symbol visibility is not configured in the `add_celix_bundle`, symbol visibility should be configured the same
way as a shared library.

```cmake
add_celix_bundle(my_bundle
    SOURCES src/my_bundle.c
    SYMBOLIC_NAME "apache_celix_my_bundle"
    NAME "Apache Celix My Bundle"
    FILENAME "celix_my_bundle"
    VERSION "1.0.0"
    GROUP "celix/my_bundle_group"
)
add_library(celix::my_bundle ALIAS my_bundle)
```

## Branch naming

- Prefix feature branches with `feature/`, hotfix branches with `hotfix/`, bugfix branches with `bugfix/` 
  and release branches with `release/`.
- If you are working on an issue, prefix the branch name with the issue number. E.g., `feature/1234-add-feature`.
- Hotfix branches are for urgent fixes that need to be applied as soon as possible.
- Use short and descriptive branch names.

## Commit Messages
    
- Utilize the imperative mood when writing commit messages (e.g., "Add feature" instead of "Adds feature" 
  or "Added feature"). This style aligns with git's auto-generated messages for merge commits or revert actions.
- Ensure that commit messages are descriptive and provide meaningful context. 
- Keep the first line of the commit message concise, ideally under 50 characters. 
  This summary line serves as a quick overview of the change and should be easy to read in git logs. 
- If more context is needed, separate the summary line from the body with a blank line. 
  The body can provide additional details, explanations, or reasoning behind the changes. 
  Aim to keep each line of the commit message body wrapped at around 72 characters for optimal readability.
- Use bullet points, numbered lists, or other formatting conventions when listing multiple changes or points in the 
  commit message body to improve readability.
- When applicable, reference related issues, bug reports, or pull requests in the commit message body to 
  provide additional context and establish connections between the commit and the larger project.
  - If your commit fixes, closes, or resolves an issue, use one of these keywords followed by the issue number 
    (e.g., "fixes #42", "closes #42", or "resolves #42"). 
  - If you want to reference an issue without closing it, simply mention the issue number 
    (e.g., "related to #42" or "#42").

## Benchmarking

- When needed, use benchmarking to measure performance.
- Use the Google Benchmark framework for benchmarking.

## Code Quality

- New code should be reviewed through a pull request and no direct commits on the master branch are allowed.
  - At least 1 reviewer should review the code.
- Hotfix pull request can be merged first and reviewed later, the rest is reviewed first and merged later.
- Unit tests should be written for all new code. 
- Code coverage should be measured and strive for a minimum of 95% code coverage.
- For existing code, maintain or increase the code coverage.
- Code should be checked for memory leaks using AddressSanitizer.
- Coverity scan are done on the master on a regular basis. Ideally new coverity issues should be fixed as soon as 
  possible.
