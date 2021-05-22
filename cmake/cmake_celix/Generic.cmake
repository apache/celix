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

function(install_celix_targets)
    install_celix_bundle_targets(${ARGN})
endfunction ()

#[[
Add bundles as dependencies to a cmake target, so that the bundle zip files are created before the cmake target is
 created.

add_celix_bundle_dependencies(<cmake_target>
    bundles...
)
]]
function(add_celix_bundle_dependencies)
    list(GET ARGN 0 TARGET)
    list(REMOVE_AT ARGN 0)
    foreach(BUNDLE_TARGET IN LISTS ARGN)
        if (TARGET ${BUNDLE_TARGET})
            get_target_property(IMPORT ${BUNDLE_TARGET} BUNDLE_IMPORTED)
            get_target_property(CREATE_BUNDLE_TARGET ${BUNDLE_TARGET} BUNDLE_CREATE_BUNDLE_TARGET)
            if (IMPORT)
                #NOP, an imported bundle target -> handle target without DEPENDS
            else ()
                add_dependencies(${TARGET} ${CREATE_BUNDLE_TARGET})
            endif ()
        endif()
    endforeach()
endfunction()