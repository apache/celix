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

## add_celix_bundle
Add a Celix bundle to the project.  There are three variants:
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
)
```

## celix_bundle_private_libs
Add libraries to a bundle. The libraries should be cmake library targets or an absolute path to an existing library.

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
 
Will result in BundleA from framework B to use the libfoo loaded in BundleA from framework A.

```CMake
celix_bundle_private_libs(<bundle_target>
    lib1 lib2 ...
)
```

## celix_bundle_files
Add files to the target bundle. DESTINATION is relative to the bundle archive root.
The rest of the command is conform file(COPY ...) cmake command.
See cmake file(COPY ...) command for more info.

Note with celix_bundle_files files are copied cmake generation time. 
Updates are not copied !

```CMake
celix_bundle_files(<bundle_target>
    files... DESTINATION <dir>
    [FILE_PERMISSIONS permissions...]
    [DIRECTORY_PERMISSIONS permissions...]
    [NO_SOURCE_PERMISSIONS] [USE_SOURCE_PERMISSIONS]
    [FILES_MATCHING]
    [ [PATTERN <pattern> | REGEX <regex>]
      [EXCLUDE] [PERMISSIONS permissions...] ] 
    [...]
)
```


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
Get bundle filename from an (imported) bundle target taking into account the
used CMAKE_BUILD_TYPE and available bundle configurations. 

```CMake
celix_get_bundle_filename(<bundle_target> VARIABLE_NAME)
```

Example: `celix_get_bundle_filename(Celix::shell SHELL_BUNDLE_FILENAME)` will result in `celix_shell.zip` for a `RelWithDebInfo` cmake build type and in `celix_shell-Debug.zip` for a `Debug` cmake build type (if the a debug bundle version exists). 

## celix_get_bundle_file
Get bundle file (absolute path to a bundle) from an (imported) bundle target taking into account the used CMAKE_BUILD_TYPE and available bundle configurations.

```CMake
celix_get_bundle_file(<bundle_target> VARIABLE_NAME)
```

Example: `celix_get_bundle_file(Celix::shell SHELL_BUNDLE_FILE)`

## celix_get_bundle_symbolic_name
Get bundle symbolic name from an (imported) bundle target.
TODO does this work on imported bundle targets? -> update this

```CMake
celix_get_bundle_symbolic_name(<bundle_target> VARIABLE_NAME)
```

Example: `celix_get_bundle_symbolic_name(Celix::shell SHELL_BUNDLE_SYMBOLIC_NAME)`

## add_celix_bundle_dependencies
Add bundles as dependencies to a cmake target, so that the bundle zip files will be created before the cmake target.

```CMake
add_celix_bundle_dependencies(<cmake_target>
    bundles...
)
```

## install_celix_bundle
Install bundle when 'make install' is executed. 
Bundles are installed at `<install-prefix>/share/<project_name>/bundles`.
Headers are installed at `<install-prefix>/include/<project_name>/<bundle_name>`
Resources are installed at `<install-prefix>/shared/<project_name>/<bundle_name>`

Optional arguments:
- EXPORT: Associates the installed bundle with a export_name. 
  The export name can be used to generate a Celix Targets cmake file (see install_celix_bundle_targets)
- PROJECT_NAME: The project name for installing. Default is the cmake project name.
- BUNDLE_NAME: The bundle name used when installing headers/resources. Default is the bundle target name.
- HEADERS: A list of headers to install for the bundle.
- RESOURCES: A list of resources to install for the bundle.

```CMake
install_celix_bundle(<bundle_target>
    [EXPORT] export_name
    [PROJECT_NAME] project_name
    [BUNDLE_NAME] bundle_name
    [HEADERS header_file1 header_file2 ...]
    [RESOURCES resource1 resource2 ...]
)
```

## install_celix_targets
Generate and install a Celix Targets cmake file which contains CMake commands to create imported targets for the bundles 
install using the provided <export_name>. These imported CMake targets can be used in in CMake project using the installed
bundles. 

Optional Arguments:
- FILE: The Celix Targets cmake filename to used, without the cmake extension. Default is <export_name>BundleTargets.
- PROJECT_NAME: The project name to used for the share location. Default is the cmake project name.
- DESTINATION: The (relative) location to install the Celix Targets cmake file to. Default is share/<PROJECT_NAME>/cmake.

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

# Celix Containers
The 'add_celix_container' Celix CMake command can be used to create Celix containers.
Celix containers are executables preconfigured with configuration properties and bundles to run.    

## add_celix_container
Add a Celix container, consisting out of a selection of bundles and a Celix launcher.
Celix containers can be used to run/test a selection of bundles in the celix framework.
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
- COPY: With this option the used bundles are copied to the container build dir in the 'bundles' dir.  
  A additional result of this is that the configured references to the bundles are then relative instead of absolute. 
- CXX: With this option the generated Celix launcher (if used) will be a C++ source instead of a C source. 
  A additional result of this is that Celix launcher is also linked against stdlibc++.
- USE_CONFIG: With this option config properties are generated in a 'config.properties' instead of embedded in the Celix launcher.
- GROUP: If configured the build location will be prefixed the GROUP. Default is empty.
- NAME: The name of the executable. Default is <celix_container_name>. Only useful for generated/LAUNCHER_SRC Celix launchers.
- DIR: The base build directory of the Celix container. Default is `<cmake_build_dir>/deploy`.
- BUNDLES: A list of bundles to configured for the Celix container to install and start. 
  These bundle will be configured for run level 3. See 'celix_container_bundles' for more info.
- PROPERTIES: A list of configuration properties, these can be used to configure the Celix framework and/or bundles. 
  Normally this will be EMBEDED_PROPERTIES, but if the USE_CONFIG option is used this will be RUNTIME_PROPERTIES. 
  See the framework library or bundles documentation about the available configuration options.
- EMBEDDED_PROPERTIES: A list of configuration properties which will be used in the generated Celix launcher. 
- RUNTIME_PROPERTIES: A list of configuration properties which will be used in the generated config.properties file.

```CMake
add_celix_container(<celix_container_name>
    [COPY]
    [CXX]
    [USE_CONFIG]
    [GROUP group_name]
    [NAME celix_container_name]
    [DIR dir]
    [BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [EMBEDDED_PROPERTIES "prop1=val1" "prop2=val2" ...]
    [RUNTIME_PROPERTIES "prop1=val1" "prop2=val2" ...]
)
```

```CMake
add_celix_container(<celix_container_name>
    LAUNCHER launcher
    [COPY]
    [CXX]
    [USE_CONFIG]
    [GROUP group_name]
    [NAME celix_container_name]
    [DIR dir]
    [BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [EMBEDDED_PROPERTIES "prop1=val1" "prop2=val2" ...]
    [RUNTIME_PROPERTIES "prop1=val1" "prop2=val2" ...]
)
```

```CMake
add_celix_container(<celix_container_name>
    LAUNCHER_SRC launcher_src
    [COPY]
    [CXX]
    [USE_CONFIG]
    [GROUP group_name]
    [NAME celix_container_name]
    [DIR dir]
    [BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [EMBEDDED_PROPERTIES "prop1=val1" "prop2=val2" ...]
    [RUNTIME_PROPERTIES "prop1=val1" "prop2=val2" ...]
)
```

## celix_container_bundles
Add the selected bundles to the Celix container. These bundles are (if configured) copied to the container build dir and
are added to the configuration properties so that they are installed and started when the Celix container executed.

The Celix framework support 7 (0 - 6) run levels. Run levels can be used to control the start and stop order of bundles.
Bundles in run level 0 are started first and bundles in run level 6 are started last. 
When stopping bundles in run level 6 are stopped first and bundles in run level 0 are stopped last.
Within a run level the order of configured decides the start order; bundles added earlier are started first.


Optional Arguments:
- LEVEL: The run level for the added bundles. Default is 3.

```CMake
celix_container_bundles(<celix_container_target_name>
    [LEVEL (0..5)]
    bundle1
    bundle2
    ...
)
```

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

# Misc Celix CMake Commands

## celix_target_embed_bundles

TODO


# Celix Docker Images
The `add_celix_docker` Apache Celix CMake command can be used to create Apache Celix docker directories.
These directories can be used (with 'docker build' or podman) to create very small Apache Celix docker images.

## add_celix_docker
Adds a docker target dir, containing a all the required executables,
libraries, filesystem files and selected bundles needed to run a Apache Celix framework in a docker container.

The 'add_celix_docker' target is a executable target and can be used to link libraries which are needed in the docker image.

The docker dir can be found in `<cmake_build_dir>/docker[/<group_name>]/<docker_name>`.

The docker directories are build with the target `celix-build-docker-dirs`, this does not create the
docker images and can also be executed on systems without docker. The `celix-build-docker-dirs` is trigger
with `make all`.

The docker images are build with the target `celix-build-docker-images`. For this to work docker needs te installed
and the user executing the `celix-build-docker-images` should have right to create docker images.
The `celix-build-docker-images` is not triggered with `make all`

There are three variants of 'add_celix_docker':
- If no launcher is specified a custom Celix launcher will be generated. This launcher also contains the configured properties.
- If a LAUNCHER_SRC is provided a Celix launcher will be build using the provided sources. Additional sources can be added with the
  CMake 'target_sources' command.
- If a LAUNCHER (absolute path to a executable of CMake `add_executable` target) is provided that will be used as Celix launcher.

Optional arguments:
- CXX: With this option the generated Celix launcher (if used) will be a C++ source instead of a C source.
  A additional result of this is that Celix launcher is also linked against stdlibc++.
- GROUP: If configured the build location will be prefixed the GROUP. Default is empty.
- NAME: The name of the executable. Default is <docker_target_name>. Only useful for generated/LAUNCHER_SRC Celix launchers.
- FROM: Configured the docker image base. Default is scratch.
  If configured a minimal filesystem will not be created!
- BUNDLES_DIR: Configures the directory where are all the bundles are copied. Default is /bundles.
- WORKDIR: Configures the workdir of the docker image. Default is /root.
- IMAGE_NAME: Configure the image name. Default is NAME.
- BUNDLES: Configures the used bundles. These bundles are configured for run level 3. see 'celix_docker_bundles' for more info.
- PROPERTIES: Configure configuration properties.
- INSTRUCTIONS: Additional dockker instruction to add the the generated Dockerfile.

```CMake
add_celix_docker(<docker_target_name>
    [CXX]
    [GROUP group_name]
    [NAME deploy_name]
    [FROM docker_from_image]
    [BUNDLES_DIR bundle_dir_in_docker_image]
    [WORKDIR workdir_in_docker_image]
    [IMAGE_NAME docker_image_name]
    [BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [INSTRUCTIONS "instr1" "instr2" ...]
)
```

```CMake
add_celix_docker(<docker_target_name>
    LAUNCHER_SRC launcher_src
    [CXX]
    [GROUP group_name]
    [NAME deploy_name]
    [FROM docker_from_image]
    [BUNDLES_DIR bundle_dir_in_docker_image]
    [WORKDIR workdir_in_docker_image]
    [IMAGE_NAME docker_image_name]
    [BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [INSTRUCTIONS "instr1" "instr2" ...]
)
```

```CMake
add_celix_docker(<docker_target_name>
    LAUNCHER launcher
    [CXX]
    [GROUP group_name]
    [NAME deploy_name]
    [FROM docker_from_image]
    [BUNDLES_DIR bundle_dir_in_docker_image]
    [WORKDIR workdir_in_docker_image]
    [IMAGE_NAME docker_image_name]
    [BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [INSTRUCTIONS "instr1" "instr2" ...]
)
```

## celix_docker_bundles
Add the selected bundles to the Celix docker image. These bundles are copied to the docker build dir and
are added to the configuration properties so that they are installed and started when the Celix docker container is created and started.

The Celix framework support 7 (0 - 6) run levels. Run levels can be used to control the start and stop order of bundles.
Bundles in run level 0 are started first and bundles in run level 6 are started last.
When stopping bundles in run level 6 are stopped first and bundles in run level 0 are stopped last.
Within a run level the order of configured decides the start order; bundles added earlier are started first.


Optional Arguments:
- LEVEL: The run level for the added bundles. Default is 3.

```CMake
celix_docker_bundles(<celix_container_target_name>
    [LEVEL (0..5)]
    bundle1
    bundle2
    ...
)
```

## celix_docker_properties
Same as `celix_container_properties`, but then for the celix container
in the docker image.

```CMake
celix_docker_properties(<docker_target_name>
    "prop1=val1"
    "prop2=val2"
    ...
)
```

## celix_docker_embedded_properties
Same as `celix_container_embedded_properties`, but then for the celix container
in the docker image.

```CMake
celix_docker_embedded_properties(<docker_target_name>
    "prop1=val1"
    "prop2=val2"
    ...
)
```

## celix_docker_instructions
Add the provided docker instruction to the end of the generated
Dockerfile.

```CMake
celix_docker_instructions(<docker_target_name>
    "instruction1"
    "instruction2"
    ...
)
```
