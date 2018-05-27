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

#Apache Celix - Subprojects

Apache Celix is organized into several subprojects. The following subproject are currently available:

* [Framework](../../libs/framework) - The Apache Celix framework, an implementation of OSGi adapted to C.
* [C Dependency Manager](../../libs/dependency_manager) - A C component/dependency model for use through an API provided as library.
* [C++ Dependency Manager](../../libs/dependency_manager_cxx) - A C++ component/dependency model for use through an API provided as library.
* [Device Access](../../bundles/device_access) - An implementation of the OSGi Device Access specification adapted to C.
* [Etcd library](../../libs/etcdlib) - A C library that interfaces with ETCD.
* [Examples](../../examples) - A Selection of examples showing how the framework can be used.
* [Log Service](../../bundles/log_service) - An implementation of the OSGi Log Service adapated to C.
* [Log Writer](../../bundles/log_writer) - A simple log writer for use in combination with the Log Service.
* [Remote Service Admin](../../bundles/remote_services) - An implementation of the OSGi Remote Service Admin Service - for several protocols - adapted to C.
    * [Remote Service Admin HTTP](../../bundles/remote_services/remote_service_admin_http) - A HTTP implementation of the RSA.
    * [Remote Service Admin DFI](../../bundles/remote_services/remote_service_admin_dfi) - A Dynamic Function Interface (DFI) implementation of the RSA.
    * [Remote Service Admin SHM](../../bundles/remote_services/remote_service_admin_shm) - A shared memory implementation of the RSA.
    * [Topology Manager](../../bundles/remote_services/topology_manager) - A (scoped) RSA Topology Manager implementation.
    * [Discovery Configured](../../bundles/remote_services/discovery_configured) - A RSA Discovery implementation using static configuration (xml).
    * [Discovery Etcd](../../bundles/remote_services/dicovery_etcd) - A RSA Discovery implementation using etcd.
    * [Discovery SHM](../../bundles/remote_services/dicovery_shm) - A RSA Discovery implementation using shared memory.
* [Shell](../../bundles/shell/shell) - A OSGi shell implementation.
* [Shell TUI](../../bundles/shell/shell_tui) - A textual UI for the Celix Shell.
* [Remote Shell](../../bundles/shell/remote_shell) - A remote (telnet) frontend for the Celix shell.
* [Bonjour Shell](../../bundles/shell/shell_bonjour) - A remote (Bonjour / mDNS) frontend for the Celix shell.
* [Deployment Admin](../../bundles/deployment_admin) - A deployment admin implementation.

