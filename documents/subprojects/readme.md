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

* [Framework](../../framework) - The Apache Celix framework, an implementation of OSGi adapted to C.
* [C Dependency Manager](../../dependency_manager) - A C component/dependency model for use through an API provided as library.
* [C++ Dependency Manager](../../dependency_manager_cxx) - A C++ component/dependency model for use through an API provided as library.
* [Device Access](../../device_access) - An implementation of the OSGi Device Access specification adapted to C.
* [Etcd library](../../etcdlib) - A C library that interfaces with ETCD. 
* [Examples](../../examples) - A Selection of examples showing how the framework can be used.
* [Log Service](../../log_service) - An implementation of the OSGi Log Service adapated to C.
* [Log Writer](../../log_writer) - A simple log writer for use in combination with the Log Service.
* [Remote Service Admin](../../remote_services) - An implementation of the OSGi Remote Service Admin Service - for several protocols - adapted to C.
    * [Remote Service Admin HTTP](../../remote_services/remote_service_admin_http) - A HTTP implementation of the RSA.
    * [Remote Service Admin DFI](../../remote_services/remote_service_admin_dfi) - A Dynamic Function Interface (DFI) implementation of the RSA.
    * [Remote Service Admin SHM](../../remote_services/remote_service_admin_shm) - A shared memory implementation of the RSA.
    * [Topology Manager](../../remote_services/topology_manager) - A (scoped) RSA Topology Manager implementation. 
    * [Discovery Configured](../../remote_services/discovery_configured) - A RSA Discovery implementation using static configuration (xml).
    * [Discovery Etcd](../../remote_services/dicovery_etcd) - A RSA Discovery implementation using etcd.
    * [Discovery SHM](../../remote_services/dicovery_shm) - A RSA Discovery implementation using shared memory.
* [Shell](../../shell) - A OSGi shell implementation.
* [Shell TUI](../../shell_tui) - A textual UI for the Celix Shell.
* [Remote Shell](../../remote_shell) - A remote (telnet) frontend for the Celix shell.
* [Bonjour Shell](../../shell_bonjour) - A remote (Bonjour / mDNS) frontend for the Celix shell.
* [Deployment Admin](../../deployment_admin) - A deployment admin implementation.

