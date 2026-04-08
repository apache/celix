---
title: Building and Developing Apache Celix with CLion
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

# Building and Developing Apache Celix with CLion
Apache Celix can be build for development in CLion with use of the Conan package manager.
Conan will arrange the building of the Apache Celix dependencies and generate config package files for these dependencies.

Conan will also generate a `conanrun.sh` and `deactivate_conanrun.sh` script that does the environment (de)setup for the 
binary locations of the build dependencies (i.e. configures `PATH` and `LD_LIBRARY_PATH`/`DYLD_LIBRARY_PATH`).

## Configuring CLion for C++17
C++ code in Celix is default configured for C++14. This is manually changed for C++17 libraries, bundles and test code
by updating the CMAKE_CXX_STANDARD var (`set(CMAKE_CXX_STANDARD 17`) in their respective CMakelists.txt files.

The downside is that CLion seems to only take into account the top level CMAKE_CXX_STANDARD value.
To ensure that CLion provides the right syntax support for C++17, add `-DCMAKE_CXX_STANDARD=17` to the `CMake Options`
in `File` -> `Settings` -> `Build, Execution, Deployment` -> `CMake`.

## Setting up the build directory
```shell
#clone git repo
git clone https://github.com/apache/celix.git
cd celix

#if needed setup conan default and debug profile
conan profile detect -f 
conan profile detect -f  --name debug
sed -i 's/build_type=Release/build_type=Debug/g' `conan profile path debug`

conan build . -pr:b default -pr:h debug -o celix/*:enable_testing=True -o celix/*:enable_address_sanitizer=True -o celix/*:build_all=True -o mosquitto/*:broker=True -o *:shared=True -b missing

#optional setup run env and run tests
ctest --preset conan-debug --verbose
```

Alternatively, issuing the following command will produce a CMakeUserPresets.json at the project root, which CLion will load automatically to set up CMake profiles, without actually building the project. Then Celix can be built within the IDE.

```shell
conan install . -pr:b default -pr:h default -s:h build_type=Debug -o celix/*:build_all=True -o celix/*:celix_cxx17=True -o celix/*:enable_testing=True -b missing  -o celix/*:enable_address_sanitizer=True -o mosquitto/*:broker=True -o *:shared=True 
```

## Configuring CLion
To ensure that all Conan build dependencies can be found the Run/Debug configurations of CLion needs te be updated.

This can be done under the menu "Run->Edit Configurations...", then select "Edit configuration templates..." and
then update the "Google Test" template so that the `conanrun.sh` Conan generated script is sourced in the 
"Environment variables" entry. 

If the Apache Celix CMake build directory is `home/joe/workspace/celix/build/Debug` then the value for 
"Environment variables" should be: `source /home/joe/workspace/celix/build/Debug/generators/conanrun.sh`

![Configure CLion](media/clion_run_configuration_template.png)
