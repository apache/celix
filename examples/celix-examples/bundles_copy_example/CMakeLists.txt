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

######################## Examples for the COPY / NO_COPY option in celix_container_bundles work #######################

#By default bundles in a container will be copied to the bundles dir in the container dir
add_celix_container(copy_bundles_example1 BUNDLES Celix::log_admin)
#The shell bundle will also be copied, because of the container copy config
celix_container_bundles(copy_bundles_example1 LEVEL 1 Celix::shell)
#The shell_tui bundle will not be copied, because of the NO_COPY option
celix_container_bundles(copy_bundles_example1 NO_COPY LEVEL 1 Celix::shell_tui)
#The log_writer_syslog bundle will be copied, because of the COPY option
celix_container_bundles(copy_bundles_example1 LEVEL 2 COPY Celix::syslog_writer)


#Bundles in this container will not be copied to the bundles dir in the container dir, because of the NO_COPY option
add_celix_container(copy_bundles_example2 NO_COPY BUNDLES Celix::log_admin)
#The shell bundle will also mpt be copied, because of the container copy config
celix_container_bundles(copy_bundles_example2 LEVEL 1 Celix::shell)
#The shell_tui bundle will not be copied, because of the NO_COPY option
celix_container_bundles(copy_bundles_example2 LEVEL 1 NO_COPY Celix::shell_tui)
#The log_writer_syslog bundle will be copied, because of the COPY option
celix_container_bundles(copy_bundles_example2 COPY LEVEL 2 Celix::syslog_writer)
