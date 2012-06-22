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
dep_opt(SHELL_TUI "Option to enable building the Shell Textual User Interface bundles" OFF DEPS SHELL)
if (BUILD_SHELL_TUI)
    bundle(shell_tui SOURCES shell_tui)
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/shell")
    target_link_libraries(shell_tui framework)
    
    package(shell_tui)
    
    INSTALL(FILES ${PROJECT_BINARY_DIR}/bundles/shell_tui.zip DESTINATION bundles COMPONENT framework)
endif (BUILD_SHELL_TUI)