# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

option(CELIX_RUST_EXPERIMENTAL "Enable experimental rust bundle" OFF)
if (CELIX_RUST_EXPERIMENTAL)
    include(FetchContent)
    FetchContent_Declare(
            Corrosion
            GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
            GIT_TAG v0.4.2
    )
    FetchContent_MakeAvailable(Corrosion)

    #Prepare a list of include paths needed to generating bindings for the Apache Celix C API
    file(GENERATE
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include_paths.txt"
            CONTENT "$<TARGET_PROPERTY:framework,INTERFACE_INCLUDE_DIRECTORIES>;$<TARGET_PROPERTY:utils,INTERFACE_INCLUDE_DIRECTORIES>"
    )

    corrosion_import_crate(MANIFEST_PATH Cargo.toml)

    corrosion_add_target_local_rustflags(rust_bundle_activator "-Cprefer-dynamic")
    corrosion_link_libraries(rust_bundle_activator Celix::framework)

    #Note corrosion_import_crate import creates a rust_bundle_activator CMake target, but this is a INTERFACE target.
    #Using the INTERFACE_LINK_LIBRARIES property we can get the actual target.
    get_target_property(ACTUAL_LIB_TARGET rust_bundle_activator INTERFACE_LINK_LIBRARIES)
    add_celix_bundle(rust_bundle ACTIVATOR ${ACTUAL_LIB_TARGET})
    add_dependencies(rust_bundle rust_bundle_activator)

    add_celix_container(rust_container
        NO_COPY
        BUNDLES
            Celix::shell
            Celix::shell_tui
            rust_bundle
    )
endif ()
