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
- With no SOURCES or ACTIVATOR a bundle without a activator will be created.

```CMake
add_celix_bundle(<bundle_target_name> 
    SOURCES source1 source2 ...
    [NAME bundle_name] 
    [SYMBOLIC_NAME bundle_symbolic_name]
    [DESCRIPTION bundle_description]
    [VERSION bundle_version]
    [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
    [EXPORT_LIBRARIES export_lib1 export_lib2 ...]
    [IMPORT_LIBRARIES import_lib1 import_lib2 ...]
    [HEADERS "header1: header1_value" "header2: header2_value" ...]
)
```

```CMake
add_celix_bundle(<bundle_target_name> 
    ACTIVATOR <activator_lib>
    [NAME bundle_name] 
    [SYMBOLIC_NAME bundle_symbolic_name]
    [DESCRIPTION bundle_description]
    [VERSION bundle_version]
    [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
    [EXPORT_LIBRARIES export_lib1 export_lib2 ...]
    [IMPORT_LIBRARIES import_lib1 import_lib2 ...]
    [HEADERS "header1: header1_value" "header2: header2_value" ...]
)
```

```CMake
add_celix_bundle(<bundle_target_name> 
    [NAME bundle_name] 
    [SYMBOLIC_NAME bundle_symbolic_name]
    [DESCRIPTION bundle_description]
    [VERSION bundle_version]
    [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
    [EXPORT_LIBRARIES export_lib1 export_lib2 ...]
    [IMPORT_LIBRARIES import_lib1 import_lib2 ...]
    [HEADERS "header1: header1_value" "header2: header2_value" ...]
)
```

- If NAME is provided that will be used as Bundle-Name. Default the bundle target name is used as symbolic name.
- If SYMBOLIC_NAME is provided that will be used as Bundle-SymbolicName. Default the bundle target name is used as symbolic name.
- If DESCRIPTION is provided that will be used as Bundle-Description. Default this is empty
- If VERSION is provided. That will be used for the Bundle-Version. In combination with SOURCES the version will alse be use to set the activator library target property VERSION and SOVERSION.
For SOVERSION only the major part is used. Expected scheme is "<major>.<minor>.<path>". Default version is "0.0.0"
- If PRIVATE_LIBRARIES is provided all provided lib are added to the "Private-Library" manifest statement and added in the root of the bundle. libraries can be cmake library targets or absolute paths to existing libraries.  
- If EXPORT_LIBRARIES is provided all provided lib are added to the "Export-Library" manifest statement and added in the root of the bundle. libraries can be cmake library targets or absolute paths to existing libraries. This is not yet supported by the celix framework.
- If IMPORT_LIBRARIES is provided all provided lib are added to the "Import-Library" manifest statement and added in the root of the bundle. libraries can be cmake library targets or absolute paths to existing libraries.  This is not yet supported by the celix framework
- If HEADERS is provided the headers values are appended to the bundle manifest.

## celix_bundle_private_libs
Add libraries to a bundle target. The libraries should be cmake library targets or an absolute path to a existing library.

```CMake
celix_bundle_private_libs(<bundle_target>
    lib1 lib2 ...
)
```

## celix_bundle_files
Add files to the target bundle. DESTINATION is relative to the bundle archive root. 
The rest of the command is conform file(COPY ...) cmake command.
See cmake file(COPY ...) command for more info.

```CMake
celix_bundle_files(<bundle_target>
    files... DESTINATION <dir>
    [FILE_PERMISSIONS permissions...]
    [DIRECTORY_PERMISSIONS permissions...]
    [NO_SOURCE_PERMISSIONS] [USE_SOURCE_PERMISSIONS]
    [FILES_MATCHING]
    [[PATTERN <pattern> | REGEX <regex>]
    [EXCLUDE] [PERMISSIONS permissions...]] [...])
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

## install_celix_bundle
Install bundle when 'make install' is executed. 
Bundles are installed at `<install-prefix>/share/<project_name>/bundles`.
Headers are installed at `<install-prefix>/include/<project_name>/<bundle_name>`
Resources are installed at `<install-prefix>/shared/<project_name>/<bundle_name>`

```CMake
install_celix_bundle(<bundle_target>
    [PROJECT_NAME] project_name
    [BUNDLE_NAME] bundle_name
    [HEADERS header_file1 header_file2 ...]
    [RESOURCES resource1 resource2 ...]
)
```

- If PROJECT_NAME is provided that will be used as project name for installing. Default is the cmake project name.
- If BUNDLE_NAME is provided that will be used as bundle for installing the headers. Default is the bundle target name.
- If HEADERS is provided the list of provided headers will be installed.
- If RESOURCES is provided the list of provided resources will be installed.

# Celix Containers

## add_celix_container
Add a Celix container, consisting out of a selection of bundles and a simple Celix launcher.
Celix containers can be used to run/test a selection of bundles in the celix framework.
A Celix container can be found in `<cmake_build_dir>/deploy[/<group_name>]/<celix_container_name>`. 
Use the `<celix_container_name>` executable to run the deployments.

```CMake
add_celix_container(<celix_container_name>
    [COPY] 
    [CXX]
    [USE_CONFIG]
    [GROUP group_name]
    [NAME celix_container_name]
    [LAUNCHER launcher]
    [DIR dir]
    [BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
)
```

- If the COPY option is provided the selected bundles will be copied in a bundles dir and the generated config.properties will use relative paths to the bundle locations. Default bundles will not be copied and the generated config.properties will use absolute references to the bundle locations.
- If CXX option is provided the celix container launcher will be build as C++ executable and as result be linked with the required C++ libraries of the used compiler.
- If USE_CONFIG option is provided a config.properties file will be generated next to the executable. The config.properties contains the configuration for the provided BUNDLES and PROPERTIES.
- If GROUP is provided the celix container will be grouped in the provided group name. 
- If NAME is provided that name will be used for the celix container dir. Default the Celix container target name will be used.
- If LAUNCHER is provided that path or target will be used as launcher executable for the Celix container. If no LAUNCHER is not provided the celix executable will be used.
- If DIR is provided, the specified dir is used instead of `<cmake_build_dir>/deploy` as deploy dir .
- If BUNDLES is provided the list of bundles will be added the the generated executable/config.properties for startup. Combined with COPY the bundles will also be copied to a bundles dir.
- If PROPERTIES is provided the list of properties will be appended to the generated executable/config.properties.

## celix_container_bundles
Add the selected bundles to the container.
The bundles are configured for auto starting.

```CMake
celix_container_bundles(<celix_container_target_name>
    [LEVEL (0..5)]
    bundle1 
    bundle2 
    ...
)
```

- If the LEVEL is provided this will be used as the bundle run level.
  If no run level is provided run level 1 is used.
  Run levels can be used to control to start order of the bundles.
  Bundles in run level 0 arre started first and bundles in run level 5
  are started last.

## celix_container_properties
Add the provided properties to the target Celix container config.properties.


```CMake
celix_container_properties(<celix_container_target_name>
    "prop1=val1" 
    "prop2=val2" 
    ...
)
```

## celix_cotainer_embedded_properties
Embeds the provided properties to the target Celix launcher as embedded properties.
Note that these properties can be overridden by using config.properties.

```CMake
celix_container_embedded_properties(<celix_container_target_name>
    "prop1=val1" 
    "prop2=val2" 
    ...
)
```

## celix_container_bundles_dir
Deploy a selection of bundles to the provided bundle dir. This can be used to create an endpoints / proxies bundles dir for the remote service admin or drivers bundles dir for the device access.

```CMake
celix_container_bundles_dir(<celix_container_target_name>
    DIR_NAME dir_name
    BUNDLES
        bundle1
        bundle2
        ...
)
```

# Celix Docker Images
It is possible the use the `add_celix_docker` Apache Celix CMake command to create Apache Celix docker directories,
which in turn can be used to create very small Apache Celix docker images.

## add_celix_docker
Adds a docker target dir, containing a all the required executables, 
libraries, filesystem files and selected bundles needed to run a Apache Celix framework in a docker container. 

The add_celix_docker target is a executable target and can be used to link libraries which are needed in the docker image.

The docker dir can be found in `<cmake_build_dir>/docker[/<group_name>]/<docker_name>`.

The docker directories are build with the target `celix-build-docker-dirs`, this does not create the
docker images and can also be executed on systems without docker. The `celix-build-docker-dirs` is trigger
with `make all`.

The docker images are build with the target `celix-build-docker-images`. For this to work docker needs te installed
and the user executing the `celix-build-docker-images` should have right to create docker images.
The `celix-build-docker-images` is not triggered with `make all`

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

- If CXX is set the entrypoint executable will be created as a C++ main file, default it is a C main file.
- If GROUP is provided the docker will be grouped in the provided group name. 
- If NAME is provided that name will be used for the docker dir. Default the deploy target name will be used.
- If FROM is provided the docker image will use the provide FROM as base, else `FROM scratch` is used and 
    a minimal filesystem will be created for the docker image
- If BUNDLES_DIR is provided that directory will be used as bundles location. Default `/bundles` will be used
- If WORKDIR is provided that directory will be used a workdir. Default `/root` will be used
- If IMAGE_NAME is provided that will be used as docker image name. Default the NAME will be used
- If BUNDLES is provided, the list of bundles will be added to the docker images and configured in the generated 
 `config.properties`
- If PROPERTIES is provided, the list of properties will added to the generated `config.properties` file
- If INSTRUCTIONS id provided, the list of docker instructions will be added the the generated `Dockerfile`

## celix_docker_bundles

Same as `celix_container_bundles`, but then for the celix container
in the docker image.

```CMake
celix_docker_bundles(<docker_target_name>
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
