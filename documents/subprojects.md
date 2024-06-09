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

# Apache Celix - Libraries and Bundles

The Apache Celix project is organized into several libraries, bundles, group of bundles and examples.

## Core Libraries
The core of Apache Celix is realized in the following libraries:

* [Framework](../libs/framework) - The Apache Celix framework, an implementation of OSGi adapted to C and C++14.
* [Utils](../libs/utils/README.md) - The Celix utils library, containing a wide range of general utils and 
                                     OSGi supporting types (properties, version, filter, string utils, file utils, etc).

## Standalone Libraries
Apache Celix also provides several standalone libraries which can be used without the framework:

* [ETCD library](../libs/etcdlib/README.md) - A C library that interfaces with ETCD.
* [Promises library](../libs/promises/README.md) - A C++17 header only adaption and implementation of the OSGi Promise specification.
* [Push Streams Library](../libs/pushstreams/README.md) - A C++17 header adaption and only implementation of the OSGi Push Stream specification.
* [Dynamic Function Interfacy Library](../libs/dfi/README.md) - A C library, build on top of libffi, to dynamical create C function proxies based on descriptor files.

When using Conan as build system, these libraries can be build standalone using the following commands:

* ETCD library: `conan create . -b missing -o build_celix_etcdlib=True`
* Promises library: `conan create . -b missing -o build_promises=True`
* Push Streams Library: `conan create . -b missing -o build_pushstreams=True`
* Dynamic Function Interfacy Library: `conan create . -b missing -o build_celix_dfi=True`

## Bundles or Groups of Bundles
Modularization is an important aspect of OSGi. Apache Celix provides several bundles or groups of bundles which extend
the Apache Celix functionality. Most of these bundles are based on the OSGi specification and are adapted to C or C++.

* [HTTP Admin](../bundles/http_admin/README.md) - An implementation for the OSGi HTTP whiteboard adapted to C and based on civetweb.
* [Log Service](../bundles/logging/README.md) - A Log Service logging abstraction for Apache Celix.
  * [Syslog Writer](../bundles/logging/log_writers/syslog_writer) - A syslog writer for use in combination with the Log Service.
* [Shell](../bundles/shell/README.md) - A OSGi C and C++ shell implementation, which can be extended with shell command services.
* [Pubsub](../bundles/pubsub/README.md) - An implementation for a publish-subscribe remote message communication system.
* [Remote Services](../bundles/remote_services/README.md) - A C adaption and implementation of the OSGi Remote Service Admin specification.
* [C++ Remote Services](../bundles/cxx_remote_services/README.md) - A C++17 adaption and implementation of the OSGi Remote Service Admin specification. Requires manually or code-generated import/export factories to work.
* [Components Ready Check](../bundles/components_ready_check/README.md) - A bundle which checks if all components are ready.

## Examples
The Apache Celix provides several [examples](../examples/celix-examples) showing how the framework and bundles can be used. These
examples are configured in such a way that they can also be used together with an installed Apache Celix.
