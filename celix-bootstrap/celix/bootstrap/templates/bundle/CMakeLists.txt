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

#{{
#import os, yaml
#bundle = None 
#with open(bundleFile) as input :
#	bundle = yaml.load(input)
#
#if not 'components' in bundle or bundle['components'] is None:
# 	bundle['components'] = []
#else:
#	for comp in bundle['components'] : 
#		if not 'serviceDependencies' in comp or comp['serviceDependencies'] is None:
#			comp['serviceDependencies'] = []
#
#}}
#{{end}}

include_directories(
        ${CELIX_INCLUDE_DIR}/dependency_manager
        "private/include"
        "public/include"
#{{ 
#for comp in bundle['components'] : 
#       for service in comp['serviceDependencies']:
#                       cog.outl("\t\"%s\"" % os.path.abspath(os.path.dirname(service['include']))) 
#}}
"org.example.mybundle" #do not edit, generated code
#{{end}}

)

add_bundle(
#{{ 
#cog.outl("\t%s" % bundle['name'])
#cog.outl("\tNAME \"%s\"" % bundle['name'])
#cog.outl("\tSYMBOLIC_NAME \"%s\"" % bundle['symbolicName']) 
#cog.outl("\tVERSION \"0.0.1\"")
#cog.outl("\t%s SOURCES") 
#cog.outl("\t\tprivate/src/bundle_activator")
#for comp in bundle['components'] :
#	cog.outl("\t\tprivate/src/%s" % comp['name'])
#}}
mybundle SOURCES #do not edit, generated code
	private/src/bundle_activator #do not edit, generated code
	private/src/example #do not edit, generated code
#{{end}}
)


#{{ 
#cog.outl("SET(BUNDLE_LIB \"%s\")" % bundle['name']) 
#}}
SET(BUNDLE_LIB "mybundle") #do not edit, generated code
#{{end}}

IF(APPLE)
target_link_libraries(${BUNDLE_LIB} ${CELIX_FRAMEWORK_LIBRARY} -Wl,-all_load dependency_manager  ${CELIX_UTILS_LIBRARY})
else()
target_link_libraries(${BUNDLE_LIB} -Wl,--whole-archive dependency_manager -Wl,--no-whole-archive ${CELIX_FRAMEWORK_LIBRARY} ${CELIX_UTILS_LIBRARY})
ENDIF()

#{{
#cog.outl("add_deploy(%s_deploy" % bundle['name'])
#cog.outl("\tCOPY")
#cog.outl("\tNAME \"%s\"" % bundle['name'])
#cog.outl("\tBUNDLES")
#cog.outl("\t\t${CELIX_BUNDLES_DIR}/shell.zip")
#cog.outl("\t\t${CELIX_BUNDLES_DIR}/dm_shell.zip")
#cog.outl("\t\t${CELIX_BUNDLES_DIR}/shell_tui.zip")
#cog.outl("\t\t${CELIX_BUNDLES_DIR}/log_service.zip")
#cog.outl("\t\t%s" % bundle['name'])
#cog.outl(")")
#}}
add_deploy(mybundle_deploy
    COPY
    NAME "mybundle"
    BUNDLES 
        mybundle
)
#{{end}}
