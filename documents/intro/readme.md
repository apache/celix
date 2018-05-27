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
Apache Celix is an implementation of the [OSGi specification](https://www.osgi.org/developer/specifications) adapted to C and C++. It is a provides a framework to develop (dynamic) modular software applications using component and/or service-oriented programming.

Apache Celix is primarily developed in C and adds an additional abstraction, in the form of a library, to support for C++. 

## Bundles
OSGi uses bundles as medium to (run-time) add and remove modules (additional functionality) to OSGi applications. For Java, OSGi bundles are jars with a OSGi specific manifest. For Apache Celix bundles are zip files containing an OSGi manifest (with some differences) and possible modules in the form of shared libraries. One of these modules can be the bundle activator in which case the Apache Celix framework will lookup the bundle create, start, stop and destroy symbols to manage the lifecycle of the bundle; This can be used bootstrap the bundles functionality. 

## C and Objects
C is a procedural programming language and as result has no direct support for the notion of a object. 
To be able to follow the OSGi specification, a standard mapping from C to Java is used. This mapping takes care of how instances, parameters, return values and exceptions (error codes) work in Apache Celix.

### Example
Before going into detail, here is an example of the mapping from a method in Java to a function in C:

```Java
//Java signature
public interface BundleContext {
    public ServiceReference[] getServiceReferences(String clazz, String filter) throws InvalidSyntaxException;
}
```

```C
//bundle_context.h

//C prototype
celix_status_t bundleContext_getServiceReferences(bundle_context_pt context, const char* serviceName, const char* filter, array_list_pt* service_references);
```

### Object methods
Using the provided example, the following templates can be extracted for mapping a object method to a C function:

```C
/**
 * 1st template
 * celix_status_t: return type of the status code
 * 
 * typeName: name of the object/type this function is part of
 * functionName: the name of the function
 * 
 * typeName_t: The actual instance to "invoke" this function on
 * parameters: default function parameters
 * output parameter: the output which the caller can use
 */
celix_status_t <typeName>_<functionName>(<typeName>_t* self [,parameters, ] [, output parameter]);

//OR

/**
* 2nd template
 * celix_status_t: return type of the status code
 * 
 * typeName: name of the object/type this function is part of
 * functionName: the name of the function
 * 
 * typeName_t: The actual instance to "invoke" this function on
 * parameters: default function parameters
 * output parameter: the output which the caller can use
 */
celix_status_t <typeName>_<functionName>(<typeName>_pt self [,parameters, ] [, output parameter]);
```

Note that although the first template is preferred, Apache Celix still uses the second template. 

Unless stated otherwise, the caller is owner of the output and should destroy/deallocate the result.
An exception is a const output parameters, this indicates the callee is still owner.

### Creating and destroying Objects
Objects in Apache Celix can generally be created and destroyed using a create and destroy functions.
For example:

```C
celix_status_t bundleContext_create(framework_pt framework, framework_logger_pt, bundle_pt bundle, bundle_context_pt *bundle_context);
celix_status_t bundleContext_destroy(bundle_context_pt context);
```

For some types a separate allocate and init and a separate deallocate and deinit are also available.
This gives a user the option the initialize and deinitialize a object on the stack. the hash_map_iterator uses this:

```C
hash_map_iterator_pt hashMapIterator_create(hash_map_pt map);
void hashMapIterator_destroy(hash_map_iterator_pt iterator);

hash_map_iterator_pt hashMapIterator_alloc(void);
void hashMapIterator_dealloc(hash_map_iterator_pt iterator);

void hashMapIterator_init(hash_map_pt map, hash_map_iterator_pt iterator);
void hashMapIterator_deinit(hash_map_iterator_pt iterator);
```

### OSGi documentation and Apache Celix
Apache Celix follows the OSGi API as close as possible, but since the OSGi specification is written primarily for Java, there will be differences (Java is OO, C is procedural).
Taking into account those differences and mapping explained before the OSGi javadoc can be used for a more in depth description of what the Apache Celix API offers. 

* [OSGi core specification 4.3](https://osgi.org/javadoc/r4v43/core/index.html)
* [OSGi compendium specification 4.3](https://osgi.org/javadoc/r4v43/cmpn/index.html)

## What is a OSGi service?
A OSGi service is a Java object register to the OSGi framework under a certain set of properties.
OSGi services are generally registered as a well known interface (using the `objectClass` property).
 
Consumers can dynamically lookup the services providing a filter to specify what kind of services their are interested in.   

## C services in Apache Celix
As mentioned OSGi uses Java Interfaces to define a service. Since C does not have Interfaces as compilable unit, this is not possible for Celix.  To be able to define a service which hides implementation details, Celix uses structs with function pointers.
 
See [Apache Celix - Getting Started Guide](https://github.com/apache/celix/blob/master/documents/getting_started/readme.md) for a more in depth look at services and service usage.
 
## Impact of dynamic services
Services in Apache Celix are dynamic, meaning that they can come and go at any moment. 
How to cope with this dynamic behaviour is very critical for creating a stable solution.
 
For Java OSGi this is already a challenge to program correctly, but less critical because generally speaking the garbage collector will arrange that objects still exists even if the providing bundle is deinstalled.
Taking into account that C has no garbage collection handling the dynamic behaviour correctly is even more critical; If a bundle providing a certain services is removed the code segment / memory allocated for the service will be removed / deallocated.
 
Apache Celix offers different solutions how to cope with this dynamic behaviour:

* Bundle Context & Service References  - This (low level) [API](../../libs/framework/public/include/bundle_context.h) exists to be compatible with the OSGi standard. This should not be used in production code, because no locking/syncing mechanisms are available.
* Service Listener - This (log level) [API](../../libs/framework/public/include/service_listener.h) can be used to retrieve event when services are being removed or are added. Combined with locking this can be used to safely monitor and use services.
* Service Tracker - This [API](../../libs/framework/public/include/service_tracker.h) can be used to register callbacks function when services are being removed or are added. Combined with locking this can be used to safely use services.
* [Dependency Manager](../../libs/dependency_manager/readme.md) - This library can be used to add service dependency is a declarative way.  A locking or syncing mechanism can be selected to safely use services. Note that this is not part of the OSGi standard.

Even though the dependency manager is not part of the OSGi specification, this is the preferred way because it uses a higher abstraction and removes a lot boilerplate code. 

## C++ Support

One of the reasons why C was chosen as implementation language is that C can act as a common denominator for (service oriented) interoperability between a range of languages.
C++ support is added with the use of a [C++ Dependency Manager](../../libs/dependency_manager_cxx/readme.md).
The Dependency Manager is arguably the most convenient way to interact with services, confers most uses cases and eliminates the necessity to port the rest of the (large) API to C++.

## Documentation

For more information see:

* [Apache Celix - Building and Installing] (../building/readme.md)
* [Apache Celix - Getting Started Guide](../getting_started/readme.md)
* [Apache Celix - CMake Commands](../cmake_commands/readme.md)
