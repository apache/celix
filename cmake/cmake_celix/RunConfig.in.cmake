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

set(RUN_CONFIG_IN "\
<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?> \n\
<launchConfiguration type=\"org.eclipse.cdt.launch.applicationLaunchType\"> \n\
    <stringAttribute key=\"bad_container_name\" value=\"${CONTAINER_NAME}\"/> \n\
    <intAttribute key=\"org.eclipse.cdt.launch.ATTR_BUILD_BEFORE_LAUNCH_ATTR\" value=\"0\"/> \n\
    <stringAttribute key=\"org.eclipse.cdt.launch.COREFILE_PATH\" value=\"\"/> \n\
    <stringAttribute key=\"org.eclipse.cdt.launch.PROGRAM_NAME\" value=\"${PROGRAM_NAME}\"/> \n\
    <stringAttribute key=\"org.eclipse.cdt.launch.PROJECT_ATTR\" value=\"${PROJECT_ATTR}\"/> \n\
    <booleanAttribute key=\"org.eclipse.cdt.launch.PROJECT_BUILD_CONFIG_AUTO_ATTR\" value=\"true\"/> \n\
    <stringAttribute key=\"org.eclipse.cdt.launch.PROJECT_BUILD_CONFIG_ID_ATTR\" value=\"org.eclipse.cdt.core.default.config.1\"/> \n\
    <stringAttribute key=\"org.eclipse.cdt.launch.WORKING_DIRECTORY\" value=\"${WORKING_DIRECTORY}\"/> \n\
    <booleanAttribute key=\"org.eclipse.cdt.launch.use_terminal\" value=\"true\"/> \n\
    <listAttribute key=\"org.eclipse.debug.core.MAPPED_RESOURCE_PATHS\"> \n\
        <listEntry value=\"/${PROJECT_ATTR}\"/> \n\
    </listAttribute> \n\
    <listAttribute key=\"org.eclipse.debug.core.MAPPED_RESOURCE_TYPES\"> \n\
        <listEntry value=\"4\"/> \n\
    </listAttribute> \n\
    <mapAttribute key=\"org.eclipse.debug.core.environmentVariables\"> \n\
    <mapEntry key=\"LD_LIBRARY_PATH\" value=\"${CELIX_LIB_DIRS}\"/> \n\
    <mapEntry key=\"DYLD_LIBRARY_PATH\" value=\"${CELIX_LIB_DIRS}\"/> \n\
    </mapAttribute> \n\
</launchConfiguration> \n\
")
