---
title: Subprojects
---

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

# Apache Celix - Subprojects

Apache Celix is organized into several subprojects. The following subproject are currently available:

* [Framework](../../libs/framework) - The Apache Celix framework, an implementation of OSGi adapted to C and C++11.
* [Etcd library](../../libs/etcdlib) - A C library that interfaces with ETCD.
* [Examples](../../examples) - A Selection of examples showing how the framework can be used.
* [Log Service](../../bundles/logging) - An implementation of the OSGi Log Service adapted to C.
* [Log Writer](../../bundles/loggin/log_writer) - A simple log writer for use in combination with the Log Service.
* [Shell](../../bundles/shell/shell/README.md) - A OSGi C and C++11 shell implementation.
* [Shell TUI](../../bundles/shell/shell_tui) - A textual UI for the Celix Shell.
* [Remote Shell](../../bundles/shell/remote_shell) - A remote (telnet) frontend for the Celix shell.
* [Bonjour Shell](../../bundles/shell/shell_bonjour) - A remote (Bonjour / mDNS) frontend for the Celix shell.
* [Web Shell](../../bundles/shell/shell_wui) - A web (websocket) frontend for the Celix shell.
* [Pubsub](../../bundles/pubsub) - An implementation for a publish-subscribe remote services system, that use dfi library for message serialization.
* [HTTP Admin](../../bundles/http_admin) - An implementation for the OSGi HTTP whiteboard adapted to C and based on civetweb.
