---
title: CMake Commands
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

# Apache Celix - CMake Commands

For Apache Celix several cmake command are added to be able to work with Apache Celix bundles and deployments.

# Bundles
A bundle is a dynamically loadable collection of shared libraries, configuration files and optional an activation entry
that can be dynamically installed and started in a Celix framework. 

## add_celix_bundle
Add a Celix bundle to the project.

```CMake
add_celix_bundle(<bundle_target_name>
        SOURCES source1 source2 ...
        [NAME bundle_name]
        [SYMBOLIC_NAME bundle_symbolic_name]
        [DESCRIPTION bundle_description]
        [GROUP bundle_group]
        [VERSION bundle_version]
        [FILENAME bundle_filename]
        [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
        [HEADERS "header1: header1_value" "header2: header2_value" ...]
        [DO_NOT_CONFIGURE_SYMBOL_VISIBILITY]
)
```

```CMake
add_celix_bundle(<bundle_target_name>
        ACTIVATOR <activator_lib>
        [NAME bundle_name]
        [SYMBOLIC_NAME bundle_symbolic_name]
        [DESCRIPTION bundle_description]
        [GROUP bundle_group]
        [VERSION bundle_version]
        [FILENAME bundle_filename]
        [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
        [HEADERS "header1: header1_value" "header2: header2_value" ...]
        [DO_NOT_CONFIGURE_SYMBOL_VISIBILITY]
)
```

```CMake
add_celix_bundle(<bundle_target_name>
        NO_ACTIVATOR
        [NAME bundle_name]
        [SYMBOLIC_NAME bundle_symbolic_name]
        [DESCRIPTION bundle_description]
        [GROUP bundle_group]
        [VERSION bundle_version]
        [FILENAME bundle_filename]
        [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
        [HEADERS "header1: header1_value" "header2: header2_value" ...]
        [DO_NOT_CONFIGURE_SYMBOL_VISIBILITY]
)
```

Example:
```CMake
add_celix_bundle(my_bundle SOURCES src/my_activator.c)
```

There are three variants:
- With SOURCES the bundle will be created using a list of sources files as input for the bundle activator library.
- With ACTIVATOR the bundle will be created using the library target or absolute path to existing library as activator library.
- With the NO_ACTIVATOR option will create a bundle without a activator (i.e. a pure resource bundle).

Optional arguments are:
- NAME: The (human readable) name of the bundle. This will be used as Bundle-Name manifest entry. Default is the  <bundle_target_name>.
- SYMBOLIC_NAME: The symbolic name of the bundle. This will be used as Bundle-SymbolicName manifest entry. Default is the <bundle_target_name>.
- DESCRIPTION: The description of the bundle. This will be used as Bundle-Description manifest entry. Default this is empty.
- GROUP: The group the bundle is part of. This will be used as Bundle-Group manifest entry. Default this is empty (no group).
- VERSION: The bundle version. This will be used for the Bundle-Version manifest entry. In combination with SOURCES the version will also be used to set the activator library target property VERSION and SOVERSION.
  For SOVERSION only the major part is used. Expected scheme is "<major>.<minor>.<path>". Default version is "0.0.0"
- FILENAME: The filename of the bundle file, without extension. Default is <bundle_target_name>. Together with the BUILD_TYPE, this will result in a filename like "bundle_target_name_Debug.zip
- PRIVATE_LIBRARIES: private libraries to be included in the bundle. Specified libraries are added to the "Private-Library" manifest statement and added in the root of the bundle. libraries can be cmake library targets or absolute paths to existing libraries.
- HEADERS: Additional headers values that are appended to the bundle manifest.
- DO_NOT_CONFIGURE_SYMBOL_VISIBILITY: By default the bundle library will be build with symbol visibility configuration preset set to hidden. This can be disabled by providing this option. 

## celix_bundle_private_libs
Add libraries to a bundle.

```CMake
celix_bundle_private_libs(<bundle_target>
    lib1 lib2 ...
)
```

Example:
```
celix_bundle_private_libs(my_bundle my_lib1 my_lib2)
```

A library should be a cmake library target or an absolute path to existing library.
The libraries will be copied into the bundle zip and activator library will be linked (PRIVATE) against them.

Apache Celix uses dlopen with RTLD_LOCAL to load the activator library in a bundle.
It is important to note that dlopen will always load the activator library,
but not always load the libraries the bundle activator library is linked against.
If the activator library is linked against a library which is already loaded, the already loaded library will be used.
More specifically dlopen will decide this based on the NEEDED header in the activator library
and the SO_NAME headers of the already loaded libraries.

For example installing in order:
- Bundle A with a private library libfoo (SONAME=libfoo.so) and
- Bundle B with a private library libfoo (SONAME=libfoo.so).
  Will result in Bundle B also using libfoo loaded from the cache dir in Bundle A.

This also applies if multiple Celix frameworks are created in the same process. For example installed in order:
- Bundle A with a private library libfoo (SONAME=libfoo.so) in Celix Framework A and
- The same Bundle A in Celix Framework B.
  Will result in Bundle A from Framework B to use the libfoo loaded from the cache dir of Bundle A in framework A.

## celix_bundle_files
Add files to the target bundle.

```CMake
celix_bundle_files(<bundle_target>
    files... DESTINATION <dir>
    [FILE_PERMISSIONS permissions...]
    [DIRECTORY_PERMISSIONS permissions...]
    [NO_SOURCE_PERMISSIONS] [USE_SOURCE_PERMISSIONS]
    [FILES_MATCHING]
    [PATTERN <pattern> | REGEX <regex>]
    [EXCLUDE] [PERMISSIONS permissions...] [...]
)
```

Example:
```CMake
celix_bundle_files(my_bundle ${CMAKE_CURRENT_LIST_DIR}/resources/my_file.txt DESTINATION META-INF/subdir)
```

DESTINATION is relative to the bundle archive root.
The rest of the command is based on the file(COPY ...) cmake command.
See cmake file(COPY ...) command for more info.

Note with celix_bundle_files, files are copied cmake generation time. Updates are not copied !!

## celix_bundle_add_dir
Copy to the content of a directory to a bundle.

```CMake
celix_bundle_add_dir(<bundle_target> <input_dir>
    [DESTINATION <relative_path_in_bundle>]
)
```

Example:
```CMake
celix_bundle_add_dir(my_bundle bundle_resources/ DESTINATION "resources")
```

Optional arguments:
- DESTINATION: Destination of the files, relative to the bundle archive root. Default is ".".

Note celix_bundle_add_dir copies the dir and can track changes.

## celix_bundle_add_files
Copy to the content of a directory to a bundle.

```CMake
celix_bundle_add_files(<bundle_target>
    [FILES <file1> <file2> ...]
    [DESTINATION <relative_path_in_bundle>]
)
```

Example:
```CMake
celix_bundle_add_files(my_bundle FILES my_file1.txt my_file2.txt DESTINATION "resources")
```

Optional arguments:
- FILES: Which files to copy to the the bundle.
- DESTINATION: Destination of the files, relative to the bundle archive root. Default is ".".

Note celix_bundle_add_files copies the files and can track changes.

## celix_bundle_headers
Append the provided headers to the target bundle manifest.

```CMake
celix_bundle_headers(<bundle_target>
    "header1: header1_value"
    "header2: header2_value"
    ...
)
```

## celix_bundle_symbolic_name
Set bundle symbolic name

```CMake
celix_bundle_symbolic_name(<bundle_target> symbolic_name)
```

## celix_get_bundle_symbolic_name
Get bundle symbolic name from a (imported) bundle target.

```CMake
celix_get_bundle_symbolic_name(<bundle_target> VARIABLE_NAME)
```

Example: `celix_get_bundle_symbolic_name(Celix::shell SHELL_BUNDLE_SYMBOLIC_NAME)`

## celix_bundle_name
Set bundle name

```CMake
celix_bundle_name(<bundle_target> name)
```

## celix_bundle_version
Set bundle version

```CMake
celix_bundle_version(<bundle_target> version)
```

## celix_bundle_description
Set bundle description

```CMake
celix_bundle_description(<bundle_target> description)
```

## celix_bundle_group
Set bundle group.

```CMake  
celix_bundle_group(<bundle_target> bundle group)
```

## celix_get_bundle_filename
Get bundle filename from a (imported) bundle target taking into account the
used CMAKE_BUILD_TYPE and available bundle configurations.

```CMake
celix_get_bundle_filename(<bundle_target> VARIABLE_NAME)
```

Example:
```CMake
celix_get_bundle_filename(Celix::shell SHELL_BUNDLE_FILENAME)
``` 

## celix_get_bundle_file
Get bundle file (absolute path to a bundle) from a (imported) bundle
target taking into account the used CMAKE_BUILD_TYPE and available
bundle configurations.

```CMake
celix_get_bundle_file(<bundle_target> VARIABLE_NAME)
```

Example:
```CMake
celix_get_bundle_file(Celix::shell SHELL_BUNDLE_FILE)
```

## install_celix_bundle
Install bundle when 'make install' is executed.

```CMake
install_celix_bundle(<bundle_target>
    [EXPORT] export_name
    [PROJECT_NAME] project_name
    [BUNDLE_NAME] bundle_name
    [HEADERS header_file1 header_file2 ...]
    [RESOURCES resource1 resource2 ...]
)
```

Bundles are installed at `<install-prefix>/share/<project_name>/bundles`.
Headers are installed at `<install-prefix>/include/<project_name>/<bundle_name>`
Resources are installed at `<install-prefix>/shared/<project_name>/<bundle_name>`

Optional arguments:
- EXPORT: Associates the installed bundle with a export_name.
  The export name can be used to generate a CelixTargets.cmake file (see install_celix_bundle_targets)
- PROJECT_NAME: The project name for installing. Default is the cmake project name.
- BUNDLE_NAME: The bundle name used when installing headers/resources. Default is the bundle target name.
- HEADERS: A list of headers to install for the bundle.
- RESOURCES: A list of resources to install for the bundle.

## install_celix_targets
Generate and install a Celix Targets cmake file which contains CMake commands to create imported targets for the bundles 
install using the provided <export_name>. These imported CMake targets can be used in CMake projects using the installed
bundles. 

```CMake
install_celix_targets(<export_name>
    NAMESPACE <namespace>
    [FILE <celix_target_filename>]
    [PROJECT_NAME <project_name>]
    [DESTINATION <celix_targets_destination>]
)
```

Example:
```CMake
install_celix_targets(celix NAMESPACE Celix:: DESTINATION share/celix/cmake FILE CelixTargets)
```

Optional Arguments:
- FILE: The Celix Targets cmake filename to used, without the cmake extension. Default is <export_name>BundleTargets
- PROJECT_NAME: The project name to used for the share location. Default is the cmake project name.
- DESTINATION: The (relative) location to install the Celix Targets cmake file to. Default is share/<PROJECT_NAME>/cmake.

# Celix Containers
Celix containers are executables preconfigured to start a Celix framework with a set of configuration properties 
and a set of bundles to install or install and start. 

## add_celix_container
Add a Celix container, consisting out of a selection of bundles and a Celix launcher.
Celix containers can be used to start a Celix framework together with a selection of bundles.

A Celix container will be build in `<cmake_build_dir>/deploy[/<group_name>]/<celix_container_name>`.
Use the `<celix_container_name>` executable to run the containers.

There are three variants of 'add_celix_container':
- If no launcher is specified a custom Celix launcher will be generated. This launcher also contains the configured properties.
- If a LAUNCHER_SRC is provided a Celix launcher will be build using the provided sources. Additional sources can be added with the
  CMake 'target_sources' command.
- If a LAUNCHER (absolute path to a executable of CMake `add_executable` target) is provided that will be used as Celix launcher.

Creating a Celix containers using 'add_celix_container' will lead to a CMake executable target (expect if a LAUNCHER is used).
These targets can be used to run/debug Celix containers from a IDE (if the IDE supports CMake).

Optional Arguments:
- COPY: With this option the bundles used in the container will be copied in and configured for a bundles directory
  next to the container executable. Only one of the COPY or NO_COPY options can be provided.
  Default is COPY.
- NO_COPY: With this option the bundles used in the container will be configured using absolute paths to the bundles
  zip files. Only one of the COPY or NO_COPY options can be provided.
  Default is COPY.
- CXX: With this option the generated Celix launcher (if used) will be a C++ source.
  This ensures that the Celix launcher is linked against stdlibc++. Only one of the C or CXX options can be provided.
  Default is CXX
- C: With this option the generated Celix launcher (if used) will be a C source. Only one of the C or CXX options can
  be provided.
  Default is CXX
- USE_CONFIG: With this option the config properties are generated in a 'config.properties' instead of embedded in
  the Celix launcher.
- GROUP: If configured the build location will be prefixed the GROUP. Default is empty.
- NAME: The name of the executable. Default is <celix_container_name>. Only useful for generated/LAUNCHER_SRC
  Celix launchers.
- DIR: The base build directory of the Celix container. Default is `<cmake_build_dir>/deploy`.
- BUNDLES: A list of bundles for the Celix container to install and start.
  These bundle will be configured for run level 3. See 'celix_container_bundles' for more info.
- INSTALL_BUNDLES: A list of bundles for the Celix container to install (but not start).
- PROPERTIES: A list of configuration properties, these can be used to configure the Celix framework and/or bundles.
  Normally this will be EMBEDED_PROPERTIES, but if the USE_CONFIG option is used this will be RUNTIME_PROPERTIES.
  See the framework library or bundles documentation about the available configuration options.
- EMBEDDED_PROPERTIES: A list of configuration properties which will be used in the generated Celix launcher.
- RUNTIME_PROPERTIES: A list of configuration properties which will be used in the generated config.properties file.

```CMake
add_celix_container(<celix_container_name>
    [COPY]
    [NO_COPY]
    [CXX]
    [C]
    [USE_CONFIG]
    [GROUP group_name]
    [NAME celix_container_name]
    [DIR dir]
    [BUNDLES <bundle1> <bundle2> ...]
    [INSTALL_BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [EMBEDDED_PROPERTIES "prop1=val1" "prop2=val2" ...]
    [RUNTIME_PROPERTIES "prop1=val1" "prop2=val2" ...]
)
```

```CMake
add_celix_container(<celix_container_name>
    LAUNCHER launcher
    [COPY]
    [NO_COPY]
    [CXX]
    [C]
    [USE_CONFIG]
    [GROUP group_name]
    [NAME celix_container_name]
    [DIR dir]
    [BUNDLES <bundle1> <bundle2> ...]
    [INSTALL_BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [EMBEDDED_PROPERTIES "prop1=val1" "prop2=val2" ...]
    [RUNTIME_PROPERTIES "prop1=val1" "prop2=val2" ...]
)
```

```CMake
add_celix_container(<celix_container_name>
    LAUNCHER_SRC launcher_src
    [COPY]
    [NO_COPY]
    [CXX]
    [C]
    [USE_CONFIG]
    [GROUP group_name]
    [NAME celix_container_name]
    [DIR dir]
    [BUNDLES <bundle1> <bundle2> ...]
    [INSTALL_BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [EMBEDDED_PROPERTIES "prop1=val1" "prop2=val2" ...]
    [RUNTIME_PROPERTIES "prop1=val1" "prop2=val2" ...]
)
```

Examples:
```CMake
#Creates a Celix container in ${CMAKE_BINARY_DIR}/deploy/simple_container which starts 3 bundles located at
#${CMAKE_BINARY_DIR}/deploy/simple_container/bundles.
add_celix_container(simple_container
    BUNDLES
        Celix::shell
        Celix::shell_tui
        Celix::log_admin
    PROPERTIES
        CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=debug
)
```

## celix_container_bundles
Add a selection of bundles to the Celix container.

```CMake
celix_container_bundles(<celix_container_target_name>
      [COPY]
      [NO_COPY]
      [LEVEL (0..6)]
      [INSTALL]
      bundle1
      bundle2
      ...
)
```

Example:
```CMake
celix_container_bundles(my_container Celix::shell Celix::shell_tui)
```

The selection of  bundles are (if configured) copied to the container build dir and
are added to the configuration properties so that they are installed and started when the Celix container is executed.

The Celix framework supports 7 (0 - 6) run levels. Run levels can be used to control the start and stop order of bundles.
Bundles in run level 0 are started first and bundles in run level 6 are started last.
When stopping bundles in run level 6 are stopped first and bundles in run level 0 are stopped last.
Within a run level the order of configured decides the start order; bundles added earlier are started first.

Optional Arguments:
- LEVEL: The run level for the added bundles. Default is 3.
- INSTALL: If this option is present, the bundles will only be installed instead of the default install and start.
  The bundles will be installed after all bundle in LEVEL 0..6 are installed and started.
- COPY: If this option is present, the bundles will be copied to the container build dir. This option overrides the
  NO_COPY option used in the add_celix_container call.
- NO_COPY: If this option is present, the install/start bundles will be configured using a absolute path to the
  bundle. This option overrides optional COPY option used in the add_celix_container call.

## celix_container_properties
Add the provided properties to the target Celix container config properties.
If the USE_CONFIG option is used these configuration properties will be added to the 'config.properties' file else they
will be added to the generated Celix launcher.

```CMake
celix_container_properties(<celix_container_target_name>
    "prop1=val1"
    "prop2=val2"
    ...
)
```

## celix_container_embedded_properties
Add the provided properties to the target Celix container config properties.
These properties will be embedded into the generated Celix launcher.

```CMake
celix_container_embedded_properties(<celix_container_target_name>
    "prop1=val1" 
    "prop2=val2" 
    ...
)
```

## celix_container_runtime_properties
Add the provided properties to the target Celix container config properties.
These properties will be added to the config.properties in the container build dir. 

```CMake
celix_container_runtime_properties(<celix_container_target_name>
    "prop1=val1" 
    "prop2=val2" 
    ...
)
```

# Celix CMake commands for generic CMake targets
Celix provides several CMake commands that operate on the generic CMake targets (executable, shared library, etc). 

Celix CMake commands for generic CMake target will always use the keyword signature (`PRIVATE`, `PUBLIC`, `INTERFACE`) 
version for linking, adding sources, etc. This means that these command will not work on targets created with
an "all-plain" CMake version command.

## add_celix_bundle_dependencies
Add bundles as dependencies to a cmake target, so that the bundle zip files will be created before the cmake target.

```CMake
add_celix_bundle_dependencies(<cmake_target>
    bundles...
)
```

```CMake
add_celix_bundle_dependencies(my_exec my_bundle1 my_bundle2)
```

## celix_target_bundle_set_definition
Add a compile-definition with a set of comma seperated bundles paths to a target and also adds the bundles as 
dependency to the target.

```CMake
celix_target_bundle_set_definition(<cmake_target>
        NAME <set_name>
        [<bundle1> <bundle2>..]
        )
```

Example:
```CMake
celix_target_bundle_set_definition(test_example NAME TEST_BUNDLES Celix::shell Celix::shell_tui)
```

The compile-definition will have the name `${NAME}` and will contain a `,` separated list of bundle paths.
The bundle set can be installed using the Celix framework util function `celix_framework_utils_installBundleSet` (C)
or `celix::installBundleSet` (C++).

Adding a compile-definition with a set of bundles can be useful for testing purpose.

## celix_target_hide_symbols
Configure the symbol visibility preset of the provided target to hidden.

This is done by setting the target properties C_VISIBILITY_PRESET to hidden, the CXX_VISIBILITY_PRESET to hidden and
VISIBILITY_INLINES_HIDDEN to ON.

```CMake
celix_target_hide_symbols(<cmake_target> [RELEASE] [DEBUG] [RELWITHDEBINFO] [MINSIZEREL])
```

Optional arguments are:
- RELEASE: hide symbols for the release build type
- DEBUG: hide symbols for the debug build type
- RELWITHDEBINFO: hide symbols for the relwithdebinfo build type
- MINSIZEREL: hide symbols for the minsizerel build type

If no optional arguments are provided, the symbols are hidden for all build types.

Example:
```CMake
celix_target_hide_symbols(my_bundle RELEASE MINSIZEREL)
```