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

# Apache Celix Dependency Manager

## Introduction

The Dependency Manager contains a static library which can be used to manage (dynamic) services on a higher abstraction level in a declarative style. 
The Apache Celix Dependency Manager is inspired by the [Apache Felix Dependency Manager](http://felix.apache.org/documentation/subprojects/apache-felix-dependency-manager.html).

## Components

Components are the main building blocks for OSGi applications. They can publish services, and/or they can have dependencies. These dependencies will influence their life cycle as component will only be activated when all required dependencies are available.

Within Apache Celix a component is expected to have a set of functions where the first argument is a handle to the component (e.g. self/this). How this is achieved is up the the user, for some examples how this can be done see the example in the Apache Celix Project. 

The Dependency Manager, as part of a bundle, shares the generic bundle life cycle explained in the OSGi specification. 
Each component you define gets its own life cycle. The component life cycle is depicted in the state diagram below.

![Component Life Cycle](doc-images/statediagram.png)

Changes in the state of the component will trigger the following life cycle callback functions:

    `init`,
    `start`,
    `stop` and
    `deinit`.

The callback functions can be specified by using the component_setCallbacks.

## DM Parts

The Dependency Manager consist out of four main parts: `DM (Dependency Manager) Activator`, `Dependency Manager`, `DM Component` and `DM Service Dependency`.

### DM Activator

The `DM Activator` implements a "normal" Celix bundle activator and depends on four functions which needs to be implemented by the user of the Depedency Manager:
 - `dm_create` : Should be used to allocated and initialize a dm activator structure. If needed this structure can be used to store object during the lifecycle of the bundle.
 - `dm_init` : Should be used to interact with the `Dependency Manager`. Here a user can components, service dependencies and provided services. 
 - `dm_destroy` : Should be used to deinitialize and deallocate objects created in the `dm_create` function.


### Dependency Manager

The `Dependency Manager` act as an entry point to add or remove DM Components. The `Dependency Manager` is provided to the `dm_init` functoin.

### DM Component

The `DM Component` manages the life cycle of a component. For example, when all required service dependencies are available the `DM Component` will call the `start` specified callback function of the component. 

The `component_setImplementation` function can be used to specify which component handle to use. 
The `component_addInterface` can be used to specify one additional service provided by the component. 
The `component_addServiceDependency` can be used to specify one additional service dependency.

### Dm Service Dependency 

The `DM Service Dependency` can be used to specify service dependencies for a component. i

When these dependencies are set to required the `DM Component` will ensure that components will only be started when all required dependencies are available and stop the component if any of the required dependencies are removed. 
This feature should prevent a lot of boiler plating code compared to using a service tracker or services references directly. 

A service dependency update strategy can also be specified. Default this strategy is set to `DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND` this strategy will stop and start (suspend) a component when any of the specified service dependencies change (are removed, added or modified). 
When correctly used this strategy removes the need for locking services during updates/invocation. See the dependency manager example for more details.

The `serviceDependency_setCallbacks` function can be used to specify the function callback used when services are added, set, removed or modified. 
The `serviceDependency_setRequired` function can be used to specify if a service dependency is required.
The `serviceDependency_setStrategy` function can be used to specify a service dependency update strategy (suspend or locking).

### Snippets

#### DM Bundle Activator

The next snippet shows a dm bundle activator and how to add components to the dependency manager.
```C

//exmpl_activator.c
#include <dm_activator.h>
#include <stdlib.h>

struct dm_exmpl_activator {
    exmpl_t* exmpl;
};

celix_status_t dm_create(bundle_context_pt context, void **userData) {                                                                                                                                             
    *userData = calloc(1, sizeof(struct dm_exmpl_activator));
    return *userData != NULL ? CELIX_SUCCESS : CELIX_ENOMEM;
}

celix_status_t dm_init(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
    celix_status_t status = CELIX_SUCCESS;
    struct dm_exmpl_activator *act = (struct dm_exmpl_activator*)userData;

    act->exmpl = exmpl_create();
    if (act->exmpl != NULL) {
        dm_component_pt cmp;
        component_create(context, "Example Component", &cmp);
        component_setImplementation(cmp, act->exmpl);

        dependencyManager_add(manager, cmp);
    } else {
        status = CELIX_ENOMEM;
    }

    return status;
}

celix_status_t dm_destroy(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
    celix_status_t status = CELIX_SUCCESS;
    struct dm_exmpl_activator *act = (struct dm_exmpl_activator*)userData;

    if (act->exmpl != NULL) {
        exmpl_destroy(act->exmpl);
    }
    free(act);

    return CELIX_SUCCESS;
}  
```

### References

For more information examples please see

- [The Dependency Manager API](public/include): The dependency manager header files
- [Getting Started: Using Service with C](../documents/getting_started/using_services_with_c.md): A introduction how to work with services using the dependency manager
- [Dm example](../examples/dm_example): A DM example.

## Dependency Manager Shell support

There is support for retrieving information of the dm components with
use of the `dm` command. This command will print all known dm component,
their state, provided interfaces and required interfaces.


## Using info

If the Celix Dependency Manager is installed, 'find_package(Celix)' will set:
 - The `Celix::dm_shell` bundle target
 - The `Celix::dependency_manger_static` library target
