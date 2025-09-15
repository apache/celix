---
title: Introduction
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

# Apache Celix Introduction

## What is Apache Celix
Apache Celix is an implementation of a dynamic service framework inspired by the 
[OSGi specification](https://www.osgi.org/developer/specifications) and adapted to C and C++ (C++17).
It is a framework to develop dynamic modular software applications using component and in-process service-oriented programming.

Apache Celix core is written in C and has a C++17 header-only API on top of the C API. 

Building applications with Apache Celix can be done by creating `bundles` which provide and use `services`; 
These `services` can be used directly or in a declarative way using `components`. 
To run a selection of bundles, an Apache Celix `container` executable can be created. An Apache Celix `containter` will  
start an Apache Celix framework and install and start the provided bundles.  

## C Patterns
Apache Celix is written in C and uses some C patterns to improve the development experience.

## Bundles
An Apache Celix Bundle is a zip file which contains a collection of shared libraries, 
configuration files and optional an activation entry. 
Bundles can be dynamically installed and started in an Apache Celix framework.

## Services
An Apache Celix Service is a pointer registered to the Apache Celix framework under a set of properties (metadata).
Services can be dynamically registered into and looked up from the Apache Celix framework.

By convention a C service in Apache Celix is a pointer to struct of function pointers and a C++ service is a pointer
(which can be provided as a `std::shared_ptr`) to an object implementing a (pure) abstract class.

## Components
Apache Celix also offers a way to create components which interact with dynamic services in declarative way. 

This removes some complexity of dynamic services by declaring service dependency and configuring the dependency 
as required or optional. 

Apache Celix components can be created using the built-in C and C++ dependency manager.

Note that the dependency manager is not part of the OSGi standard, and it is inspired by the 
Apache Felix Dependency Manager. 

## Containers
Apache Celix Containers are executables which starts an Apache Celix framework, with a set of preconfigured properties 
and a set of preconfigured bundles. 
Although it is also possible to create and start a Celix framework in code, the benefit of a Celix container 
is that this can be done with a single `add_celix_container` Apache Celix CMake command. 

## C++ Support

One of the reasons why C was chosen as implementation language is that C can act as a common denominator for 
(service oriented) interoperability between a range of languages.

C++ support is build on top of the C API and is realized using a header only implementation. 
This means that all the binary artifact for the Apache Celix framework and util library are pure C and do not depend on 
libstdc++. 

Apache Celix also offers some C++ only libraries and bundles. The C++ libraries are also header only, but the C++
bundles contains binaries depending on the stdlibc++ library.

## More information

* Building
  * [Building and Installing Apache Celix](building/README.md)
* C Patterns
  * [Apache Celix C Patterns](c_patterns.md)
* Utils
  * [Apache Celix Properties & Filter](properties_and_filter.md)
  * [Apache Celix Properties Encoding](properties_encoding.md)
* Framework 
  * [Apache Celix Bundles](bundles.md)
  * [Apache Celix Services](services.md)
  * [Apache Celix Components](components.md) 
  * [Apache Celix Framework](framework.md)
  * [Apache Celix Containers](containers.md)
  * [Apache Celix Patterns](patterns.md)
  * [Apache Celix Scheduled Events](scheduled_events.md)
* [Apache Celix CMake Commands](cmake_commands)
* [Apache Celix Sub Projects](subprojects.md)
* [Apache Celix Coding Conventions Guide](development/README.md)
