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
Apache Celix is an implementation of the [OSGi specification](https://www.osgi.org/developer/specifications) 
adapted to C and C++17. 
It is a provides a framework to develop dynamic modular software applications using components and in-process services.

Apache Celix core is written in C and has a C++17 header-only API on top of the C API. 

Building applications with Apache Celix can be done by creating `bundles` which provide and use `services`; 
These `services` can be directly used or in a declarative way using `components`. 
To run a selection of bundles, a Celix `container` executable can be created. A Celix `containter` will  
start a Celix framework and install and start the provided bundles.  

## Bundles
An Apache Celix Bundle is a zip file which contains a dynamically loadable collection of shared libraries, 
configuration files and optional an activation entry. 
Bundles can be dynamically installed and started in a Celix framework.

## Services
An Apache Celix Service is a pointer registered to the Celix framework under a set of properties (metadata).
Services can be dynamically registered into and looked up from the Celix framework.

By convention a C service in Apache Celix is a pointer to struct of function pointers and a C++ service is a pointer
(which can be provided as a `std::shared_ptr`) to an object implementing a (pure) abstract class.

## Components
Apache Celix also offers a way to create components which interact with dynamic services in declarative way. 

This removes some complexity of dynamic services by declaring service dependency and configure the dependency 
as required or optional. 

Celix components can be created using the built-in C and C++ dependency manager.

Note that the dependency manager is not part of the OSGi standard, and it is inspired by the 
Apache Felix Dependency Manager. 
Even though the dependency manager is not part of the OSGi specification, 
this is the preferred way of handling dynamics services in Celix, because it uses a higher abstraction.

## Containers
Apache Celix containers are executable which start a Celix framework and a preconfigured set of bundles. 
Although it is also possible to create and start a Celix framework in code, the benefit of a Celix container 
is that this can be done with a single `add_celix_container` Celix CMake command. 

## C++ Support

One of the reasons why C was chosen as implementation language is that C can act as a common denominator for 
(service oriented) interoperability between a range of languages.

C++ (C++17) support is build on top of the C API and is realized using a header only implementation. 
This means that all the binary artifact for the Apache Celix framework and util library are pure C and do not depend on 
libstdc++. 

Apache Celix also offers some C++ only libraries and bundles. The C++ libraries are also header only, but the C++
bundles contains binaries depending on the stdlibc++ library.

## Documentation

For more information see:

* [Apache Celix - Building and Installing](../building/README.md)
* [Apache Celix - Bundles](bundles.md)
* [Apache Celix - Services](services.md)
* [Apache Celix - Components](components.md)
* TODO MAYBE add Creating Containers readme
* [Apache Celix - Getting Started Guide](../getting_started/README.md) TODO MAYBE REMOVE, replaced with bundles.md, services.md
* [Apache Celix - CMake Commands](../cmake_commands/README.md)
