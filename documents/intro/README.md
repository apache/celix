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
Apache Celix is an implementation of the [OSGi specification](https://www.osgi.org/developer/specifications) adapted to C and C++11. It is a provides a framework to develop (dynamic) modular software applications using components and dynamic services.

Apache Celix core is written in C and has a C++11 on top of the C API. 

## Bundles
OSGi uses bundles as medium to (run-time) add and remove modules (additional functionality) to OSGi applications. For Java, OSGi bundles are jars with a OSGi specific manifest. For Apache Celix bundles are zip files containing an OSGi manifest (with some differences) and possible modules in the form of shared libraries. One of these modules can be the bundle activator in which case the Apache Celix framework will lookup the bundle create, start, stop and destroy symbols to manage the lifecycle of the bundle; This can be used bootstrap the bundles functionality.

## What is a OSGi service?
A OSGi service is a Java object register to the OSGi framework under a certain set of properties.
OSGi services are generally registered as a well known interface (using the `objectClass` property).
 
Consumers can dynamically lookup the services providing a filter to specify what kind of services their are interested in.

## C services in Apache Celix
As mentioned OSGi uses Java Interfaces to define a service. Since C does not have Interfaces as compilable unit, this is not possible for Celix. To be able to define a service which hides implementation details, Celix uses structs with function pointers.

## C++ services in Apache Celix
For C++ services can be struct with function pointers (C interfaces) or C++ objects. 
For C++ objects the guideline is to provide service using pure virtual classes as interfaces. 

See [Apache Celix - Getting Started Guide](../getting_started/README.md) for a more in depth look at services and service usage.
 
## Impact of dynamic services
Services in Apache Celix are dynamic, meaning that they can come and go at any moment. 
How to cope with this dynamic behaviour is very critical for creating a stable solution.
 
For Java OSGi this is already a challenge to program correctly, but less critical because generally speaking the garbage collector will arrange that objects still exists even if the providing bundle is deinstalled.
Taking into account that C and C++ has no garbage collection handling the dynamic behaviour correctly is even more critical; If a bundle providing a certain services is removed the code segment / memory allocated for the service will be removed / deallocated.
 
Apache Celix offers different solutions how to cope with this dynamic behaviour:

* A built-in abstraction to use use services with callbacks function so that the Celix framework can ensure the services are not removed during callback execution. See `celix_bundleContext_useService(s)` and `celix::BundleContext::useService(s)` for more info. 
* Service trackers which ensure that service can only complete their unregistration when all service 
  remove callbacks have been processed. See `celix_bundleContext_trackServices` and `celix::BundleContext::trackServices` for more info.
* A built-in dependency manager abstraction where service dependency are configured declarative.  A locking or syncing mechanism can be selected to safely use services. Note that this is not part of the OSGi standard, but is inspired by the Apache Felix Dependency Manager. Even though the dependency manager is not part of the OSGi specification, this is the preferred way of creating component, because it uses a higher abstraction and removes a lot boilerplate code. 

## C++ Support

One of the reasons why C was chosen as implementation language is that C can act as a common denominator for (service oriented) interoperability between a range of languages.

C++ (C++11) support is also available both this includes a complete BundleContext abstraction and the C++  Dependency Manager.

## Documentation

For more information see:

* [Apache Celix - Building and Installing](../building/README.md)
* [Apache Celix - Getting Started Guide](../getting_started/README.md)
* [Apache Celix - CMake Commands](../cmake_commands/README.md)
