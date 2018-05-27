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

# Apache Celix C++ Dependency Manager

## Introduction

The C++ Dependency Manager contains a static library which can be used to manage (dynamic) services on a higher abstraction level in a declarative style. 
The Apache Celix C++ Dependency Manager is inspired by the [Apache Felix Dependency Manager](http://felix.apache.org/documentation/subprojects/apache-felix-dependency-manager.html).

The C++ Dependency Manager uses fluent interface to make specifying DM components and service dependencies very concise and relies on features introduced in C++11.

## C++ and C Dependency Manager

The C++ Dependency Manager is build on top of the C Dependency Manager.
To get a good overview of the C++ Dependency Manager please read the [Dependency Manager documentation](../dependency_manager/README.md)

## DM Parts

The C++ Dependency Manager consist out of four main parts: `celix::dm::DmActivator`, `celix::dm::DependencyManager`, `celix::dm::Component` and `celix::dm::ServiceDependency`.

### DmActivator

The `DmActivator` class should be inherited by a bundle specific Activator. 

- The static `DmActivator::create` method needs to be implemented and should return a bundle specific subclass instance of the DmActivator.
- The `DmActivator::init` method should be overridden and can be used to specify which components to use in the bundle.
- The `DmActivator::deinit` method can be overridden if some cleanup is needed when a bundle is stopped.

### Dependency Manager

The `DependencyManager` act as an entry point to create (DM) Components.

### Component

The (DM) `Component` manages the life cycle of a component (of the template type T). For example, when all required service dependencies are available the `Component` will call the `start` specified callback function of the component.

- The `Component::setInstance` method can be used to set the component instance to used. If no instance is set the (DM) `Component` will (lazy) create a component instance using the default constructor.
- The `Component::addInterface` method can be used to specify one additional C++ service provided by the component.
- The `Component::addCInterface` method can be used to specify one additional C service provided by the component.
- The `Component::createServiceDependency` method can be used to specify one additional typed C++ service dependency.
- The `Component::createCServiceDependency` method can be used to specify one additional typed C service dependency.

### ServiceDependency and CServiceDependency

The (DM) `ServiceDependency` can be used to specify C++ service dependencies for a component and the (DM) `CServiceDependency` can be used to specify C service dependencies for a component.

When these dependencies are set to required the `Component` will ensure that components will only be started when all required dependencies are available and stop the component if any of the required dependencies are removed.
This feature should prevent a lot of boiler plating code compared to using a service tracker or services references directly. 

A service dependency update strategy can also be specified (suspend or locking. Default this strategy is set to `DependencyUpdateStrategy::suspend` this strategy will stop and start (suspend) a component when any of the specified service dependencies changes (are removed, added or modified).
When correctly used this strategy removes the need for locking services during updates/invocation. See the dependency manager_cxx example for more details.

- The `(C)ServiceDependency::setCallbacks` methods can be used to specify the function callback used when services are added, set, removed or modified. 
- The `(C)ServiceDependency::setRequired` methods can be used to specify if a service dependency is required.
- The `(C)ServiceDependency::setStrategy` methods can be used to specify the service dependency update strategy (suspend or locking).

### References

For more information examples please see

- [The C++ Dependency Manager API](include/celix/dm): The c++ dependency manager header files
- [Dm C++ example](../examples/dm_example_cxx): A DM C++ example.
- [Getting Started: Using Services with C++](../documents/getting_started/using_services_with_cxx.md): A introduction how to work with services using the C++ dependency manager

## Using info

If the Celix C++ Dependency Manager is installed, 'find_package(Celix)' will set:
 - The `Celix::shell_api` interface (i.e. headers only) library target
 - The `Celix::shell` bundle target
