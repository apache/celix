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

Apache Celix is organized into several subprojects. The following subproject are available:

* [Framework](../libs/framework) - The Apache Celix framework, an implementation of OSGi adapted to C and C++11.
* [Utils](../libs/utils/README.md) - The Celix utils library, containing a wide range of utils functions (string, file, C hashmap, C arraylist, etc)
* [Examples](../examples) - A Selection of examples showing how the framework can be used.
* [Log Service](../bundles/logging/README.md) - A Log Service logging abstraction for Apache Celix.
  * [Syslog Writer](../bundles/logging/log_writers/syslog_writer) - A syslog writer for use in combination with the Log Service.
* [Shell](../bundles/shell/README.md) - A OSGi C and C++11 shell implementation.
* [Pubsub](../bundles/pubsub/README.md) - An implementation for a publish-subscribe remote message communication system. 
* [HTTP Admin](../bundles/http_admin/README.md) - An implementation for the OSGi HTTP whiteboard adapted to C and based on civetweb.
* [Remote Services](../bundles/cxx_remote_services) - A C++17 adaption and implementation of the OSGi Remote Service Admin specification.

Standalone libraries:

* [Etcd library](../libs/etcdlib/README.md) - A C library that interfaces with ETCD.
* [Promises library](../libs/promises/README.md) - A C++17 header only adaption and implementation of the OSGi Promise specification.
* [Push Streams Library](../libs/pushstreams/README.md) - A C++17 header adaption and only implementation of the OSGi Push Stream specification. 

