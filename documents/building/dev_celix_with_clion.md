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
Conan will arrange the building of the Apache Celix dependencies and generate Find<package> files for these dependencies.

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
conan profile new default --detect
conan profile new debug --detect
conan profile update settings.build_type=Debug debug

#generate and configure cmake-build-debug directory
conan install . celix/2.3.0 -pr:b default -pr:h debug -if cmake-build-debug/ -o celix:enable_testing=True -o celix:enable_address_sanitizer=True -o celix:build_all=True -b missing

#invoke the exact cmake command `conan install` shows to configure the build directory
cd cmake-build-debug
cmake .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=/home/peng/Downloads/git/mycelix/cmake-build-debug/conan_toolchain.cmake -DENABLE_TESTING=ON -DENABLE_CODE_COVERAGE=OFF -DENABLE_ADDRESS_SANITIZER=ON -DENABLE_UNDEFINED_SANITIZER=OFF -DENABLE_THREAD_SANITIZER=OFF -DENABLE_TESTING_DEPENDENCY_MANAGER_FOR_CXX11=OFF -DENABLE_TESTING_FOR_CXX14=OFF -DBUILD_ALL=ON -DBUILD_DEPLOYMENT_ADMIN=ON -DBUILD_HTTP_ADMIN=ON -DBUILD_LOG_SERVICE=ON -DBUILD_LOG_HELPER=ON -DBUILD_LOG_SERVICE_API=ON -DBUILD_SYSLOG_WRITER=ON -DBUILD_PUBSUB=ON -DBUILD_PUBSUB_WIRE_PROTOCOL_V1=ON -DBUILD_PUBSUB_WIRE_PROTOCOL_V2=ON -DBUILD_PUBSUB_JSON_SERIALIZER=ON -DBUILD_PUBSUB_AVROBIN_SERIALIZER=ON -DBUILD_PUBSUB_PSA_ZMQ=ON -DBUILD_PUBSUB_EXAMPLES=ON -DBUILD_PUBSUB_INTEGRATION=ON -DBUILD_PUBSUB_PSA_TCP=ON -DBUILD_PUBSUB_PSA_UDP_MC=ON -DBUILD_PUBSUB_PSA_WS=ON -DBUILD_PUBSUB_DISCOVERY_ETCD=ON -DBUILD_CXX_REMOTE_SERVICE_ADMIN=ON -DBUILD_CXX_RSA_INTEGRATION=ON -DBUILD_REMOTE_SERVICE_ADMIN=ON -DBUILD_RSA_REMOTE_SERVICE_ADMIN_DFI=ON -DBUILD_RSA_DISCOVERY_COMMON=ON -DBUILD_RSA_DISCOVERY_CONFIGURED=ON -DBUILD_RSA_DISCOVERY_ETCD=ON -DBUILD_RSA_REMOTE_SERVICE_ADMIN_SHM_V2=ON -DBUILD_RSA_JSON_RPC=ON -DBUILD_RSA_DISCOVERY_ZEROCONF=ON -DBUILD_SHELL=ON -DBUILD_SHELL_API=ON -DBUILD_REMOTE_SHELL=ON -DBUILD_SHELL_BONJOUR=ON -DBUILD_SHELL_TUI=ON -DBUILD_SHELL_WUI=ON -DBUILD_COMPONENTS_READY_CHECK=ON -DBUILD_EXAMPLES=ON -DBUILD_CELIX_ETCDLIB=ON -DBUILD_LAUNCHER=ON -DBUILD_PROMISES=ON -DBUILD_PUSHSTREAMS=ON -DBUILD_EXPERIMENTAL=ON -DBUILD_CELIX_DFI=ON -DBUILD_DEPENDENCY_MANAGER=ON -DBUILD_DEPENDENCY_MANAGER_CXX=ON -DBUILD_FRAMEWORK=ON -DBUILD_RCM=ON -DBUILD_UTILS=ON -DCELIX_CXX14=ON -DCELIX_CXX17=ON -DCELIX_INSTALL_DEPRECATED_API=ON -DCELIX_USE_COMPRESSION_FOR_BUNDLE_ZIPS=ON -DENABLE_CMAKE_WARNING_TESTS=OFF -DENABLE_TESTING_ON_CI=OFF -DFRAMEWORK_CURLINIT=ON -DBUILD_ERROR_INJECTOR_MDNSRESPONDER=ON -DCELIX_ERR_BUFFER_SIZE=512 -DCMAKE_EXE_LINKER_FLAGS=-Wl,--unresolved-symbols=ignore-in-shared-libs -DCELIX_MAJOR=2 -DCELIX_MINOR=3 -DCELIX_MICRO=0 -DCMAKE_POLICY_DEFAULT_CMP0091=NEW -DCMAKE_BUILD_TYPE=Debug

#optional build, you may want to skip it and use CLion to build
make -j

# if you don't like the above very long cmake command, you may use the following command instead
# Note that it does a full building, which may take a long time
# conan build . -bf cmake-build-debug/


#optional setup run env and run tests
source conanrun.sh 
ctest --verbose
source deactivate_conanrun.sh 
```

## Configuring CLion
To ensure that all Conan build dependencies can be found the Run/Debug configurations of CLion needs te be updated.

This can be done under the menu "Run->Edit Configurations...", then select "Edit configuration templates..." and
then update the "Google Test" template so that the `conanrun.sh` Conan generated script is sourced in the 
"Environment variables" entry. 

If the Apache Celix CMake build directory is `home/joe/workspace/celix/cmake-build-debug` then the value for 
"Environment variables" should be: `source /home/joe/workspace/celix/cmake-build-debug/conanrun.sh`

![Configure CLion](media/clion_run_configuration_template.png)
