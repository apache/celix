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

celix_subproject(FRAMEWORK "Option to build the framework shared library" "ON" DEPS UTILS)
if (FRAMEWORK) 
    cmake_minimum_required(VERSION 2.6)
    
    find_package(ZLIB REQUIRED)
    
    include(CPackComponent)
    
    #cpack_add_component(framework
    #	DISPLAY_NAME Framework
    #    DESCRIPTION "The Apache Celix framework library"
    #    REQUIRED
    #)
    
    CELIX_ADD_COMPONENT(framework
    	DISPLAY_NAME Framework
        DESCRIPTION "The Apache Celix framework library"
        GROUP all
    )
    
    add_definitions(-DUSE_FILE32API)
    include_directories(${ZLIB_INCLUDE_DIR})
    include_directories("private/include")
    include_directories("public/include")
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")

	if(WIN32)
	  set(IO private/src/iowin32.c)
	endif(WIN32)

    add_library(celix_framework SHARED
	 private/src/attribute.c private/src/bundle.c private/src/bundle_archive.c private/src/bundle_cache.c
	 private/src/bundle_context.c private/src/bundle_revision.c private/src/capability.c private/src/celix_errorcodes.c
	 private/src/filter.c private/src/framework.c private/src/manifest.c private/src/ioapi.c
	 private/src/manifest_parser.c private/src/miniunz.c private/src/module.c private/src/properties.c 
	 private/src/requirement.c private/src/resolver.c private/src/service_reference.c private/src/service_registration.c 
	 private/src/service_registry.c private/src/service_tracker.c private/src/service_tracker_customizer.c
	 private/src/unzip.c private/src/utils.c private/src/version.c private/src/version_range.c private/src/wire.c

	 private/include/attribute.h public/include/framework_exports.h

	 public/include/framework.h public/include/properties.h public/include/bundle_context.h public/include/bundle.h
	 public/include/bundle_activator.h public/include/service_registration.h public/include/service_reference.h
	 public/include/bundle_archive.h public/include/utils.h public/include/module.h public/include/service_tracker.h
	 public/include/service_tracker_customizer.h
	 
		${IO}
	 
	)
    target_link_libraries(celix_framework celix_utils ${ZLIB_LIBRARY} ${APR_LIBRARY} ${APRUTIL_LIBRARY})
    
    install(TARGETS celix_framework DESTINATION lib COMPONENT framework)
    FILE(GLOB files "public/include/*.h")
    INSTALL(FILES ${files} DESTINATION include/celix COMPONENT framework)
    
endif (FRAMEWORK)
