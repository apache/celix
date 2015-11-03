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
celix_subproject(SCR "Option to build the SCR static library" OFF DEPS LAUNCHER FRAMEWORK SHELL SHELL_TUI)
if (SCR)
    if(NOT ${WITH_APR})
		message(FATAL_ERROR "SCR requires APR, enable WITH_APR option.")
	endif()
    add_subdirectory(scr)
    add_subdirectory(test)
    
    deploy(scr-test BUNDLES shell shell_tui scr_test scr)
endif (SCR)
   
