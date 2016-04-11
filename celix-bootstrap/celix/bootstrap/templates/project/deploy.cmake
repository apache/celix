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
#import fnmatch
#import os
#import yaml
#
#project = None 
#bundle = None 
#
#with open(projectFile) as input :
#	project = yaml.load(input)
#
#cog.outl("deploy( \"%s\" BUNDLES" % project['name'])
#cog.outl("\t${CELIX_BUNDLES_DIR}/shell.zip")
#cog.outl("\t${CELIX_BUNDLES_DIR}/dm_shell.zip")
#cog.outl("\t${CELIX_BUNDLES_DIR}/shell_tui.zip")
#cog.outl("\t${CELIX_BUNDLES_DIR}/log_service.zip")
#
#
#for root, dirs, filenames in os.walk('.'):
#	for foundFile in fnmatch.filter(filenames, 'bundle.yaml'):
#		bundleFile = root + '/' + foundFile
#		with open(bundleFile) as input :
#                       bundle = yaml.load(input)
#                       cog.outl("\t%s" % bundle['name'])
#
#cog.outl(")");
#}}
#{{end}}
