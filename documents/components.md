---
title: Apache Celix Components
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

TODO refactor this documentation file
TODO also describes when cmp is suspended: there is a svc dep with suspend-strategy and the set or add/rem
callback is configured.

# Apache Celix Components
In Apache Celix, components are plain old C/C++ objects (POCOs) managed by the Apache Celix Dependency Manager (DM).
Components can provide service and have services dependencies. Components are configured declarative using the DM api.

Service dependencies will influence the 
component's lifecycle as a component will only be active when all required dependencies 
are available.   
The DM is responsible for managing the component's service dependencies, the component's lifecycle and when 
to register/unregister the component's provided services.

Note that the Apache Celix Dependency Manager is inspired by the [Apache Felix Dependency Manager](http://felix.apache.org/documentation/subprojects/apache-felix-dependency-manager.html), adapted to Apache Celix 
and the C/C++ usage.

# Component Lifecycle
Each DM Component you define gets its own lifecycle. 
A component's lifecycle state model is depicted in the state diagram below.

![Component Life Cycle](diagrams/component_lifecycle.png)

The DM can be used to configure a component's lifecycle callbacks; Lifecycle callbacks are always called from the
Celix event thread. 
The following component's lifecycle callbacks can be configured:

 - `init`
 - `start`
 - `stop`
 - `deinit`

These callbacks are used in the intermediate component's lifecycle states `Initializing`, `Starting`, `Suspending`, `Resuming`, `Stopping` and `Deinitializing`.

A DM Component has the following lifecycle states:
- `Inactive`: _The component is inactive and the DM is not managing the component yet._
- `Waiting For Required`: _The component is waiting for required service dependencies._
- `Initializing`: _The component has found its required dependencies and is initializing: 
  Calling the `init` callback._
- `Initialized And Waiting For Required`: _The component has been initialized, but is waiting for required
  dependencies._
  _Note: that this can mean: that during `init` callback new service dependencies was added or that the component
  was active, but a required service dependency is removed and as result the component is not active anymore._
- `Starting`: _The component has found its required dependencies and is starting: Calling the `start` callback and
  registering it's provided services.
- `Tracking Optional`: _The component has found its required dependencies and is started. It is still tracking for
  additional optional and required services._
- `Suspending`: _The component has found its required dependencies, but is suspending to prepare for a service change:
  Unregistering its provided service and calling the `stop` callback._
- `Suspended`: _The component has found its required dependencies and is suspended so that a service change can be
  processed._
- `Resuming`: _The component has found its required dependencies, a service change has been processed, and it is
  resuming: Calling the `start` callback and registering its provided services.
- `Stopping`: _The component has lost one or more of its required dependencies and is stopping: Unregistering its
  provided service and calling the `stop` callback._
- `Deinitializing`: _The component is being removed and is deinitializing: Calling the `deinit` callback._

## Example: Create and configure component's lifecycle callbacks in C
The following example shows how a simple component can be created and managed with the DM in C.
Because the component's lifecycle is managed by the DM, this also means that if configured correctly no additional
code is needed to remove and destroy the DM component and its implementation. 

Remarks for the C example:
 1. Although this is a C component. The simple component functions have been design for a component approach,
    using the component pointer as first argument. 
 2. The component implementation can be any POCO, as long as its lifecycle and destroy function follow a
    component approach (one argument with the component implementation pointer as type)
 3. Creates the DM component, but note that this component is not yet known to the DM. This makes it possible to
    first configure the DM component over multiple function calls, before adding - and as result 
    activating - the DM component to the DM.
 4. Configures the component implementation in the DM component, so that the implementation pointer can be used 
    in the configured component callbacks.
 5. Configures the component lifecycle callbacks to the DM Component. These callbacks should accept the component 
    implementation as its only argument. The `CELIX_DM_COMPONENT_SET_CALLBACKS` marco is used instead of the 
    `celix_dmComponent_setCallbacks` function so that the component implementation type can directly be used 
    in the lifecycle callbacks (instead of `void*`). 
 6. Configures the component destroy implementation callback to the Dm Component. This callback will be called when 
    the DM component is removed from the DM and has become inactive. The callback will be called from the Celix event
    thread. The advantages of configuring this callback is that the DM manages when the callback needs to be called; 
    this removes some complexity for the users. The `CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION` marco 
    is used instead of the `celix_dmComponent_setImplementationDestroyFunction` function so that the component
    implementation type can be directly used in the callback (instead of `void*`).
 7. Adds the DM Component the DM and as result the DM will activate manage the component.
 8. No additional code is needed to clean up components and as such no activator stop callback function needs to be 
    configured. The generated bundle activator will ensure that  all component are removed from the DM when the 
    bundle is stopped and the DM will ensure that the components are deactivated and destroyed correctly.
    
```C
//src/simple_component_activator.c
#include <stdio.h>
#include <celix_api.h>

//********************* COMPONENT *******************************/

typedef struct simple_component {
    int transitionCount; //not protected, only updated and read in the celix event thread.
} simple_component_t;

static simple_component_t* simpleComponent_create() {
    simple_component_t* cmp = calloc(1, sizeof(*cmp));
    cmp->transitionCount = 1;
    return cmp;
}

static void simpleComponent_destroy(simple_component_t* cmp) {
    free(cmp);
}

static int simpleComponent_init(simple_component_t* cmp) { // <------------------------------------------------------<1>
    printf("Initializing simple component. Transition nr %i\n", cmp->transitionCount++);
    return 0;
}

static int simpleComponent_start(simple_component_t* cmp) {
    printf("Starting simple component. Transition nr %i\n", cmp->transitionCount++);
    return 0;
}

static int simpleComponent_stop(simple_component_t* cmp) {
    printf("Stopping simple component. Transition nr %i\n", cmp->transitionCount++);
    return 0;
}

static int simpleComponent_deinit(simple_component_t* cmp) {
    printf("De-initializing simple component. Transition nr %i\n", cmp->transitionCount++);
    return 0;
}


//********************* ACTIVATOR *******************************/

typedef struct simple_component_activator {
    //nop
} simple_component_activator_t;

static celix_status_t simpleComponentActivator_start(simple_component_activator_t *act, celix_bundle_context_t *ctx) {
    //creating component
    simple_component_t* impl = simpleComponent_create(); // <--------------------------------------------------------<2>

    //create and configuring component and its lifecycle callbacks using the Apache Celix Dependency Manager
    celix_dm_component_t* dmCmp = celix_dmComponent_create(ctx, "simple_component_1"); // <--------------------------<3>
    celix_dmComponent_setImplementation(dmCmp, impl); // <-----------------------------------------------------------<4>
    CELIX_DM_COMPONENT_SET_CALLBACKS(
            dmCmp,
            simple_component_t,
            simpleComponent_init,
            simpleComponent_start,
            simpleComponent_stop,
            simpleComponent_deinit); // <----------------------------------------------------------------------------<5>
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(
            dmCmp,
            simple_component_t,
            simpleComponent_destroy); // <---------------------------------------------------------------------------<6>

    //Add dm component to the dm.
    celix_dependency_manager_t* dm = celix_bundleContext_getDependencyManager(ctx);
    celix_dependencyManager_add(dm, dmCmp); // <---------------------------------------------------------------------<7>
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(simple_component_activator_t, simpleComponentActivator_start, NULL) // <------------------<8>
```

## Example: Create and configure component's lifecycle callbacks in C++
The following example shows how a simple component can be created and managed with the DM in C++. 
For C++ the DM will manage the components and also ensures that component implementations are kept in scope for as
long as the components are managed by the DM. 

Remarks for the C++ example:
1. For C++ the DM can directly work on classes and as result lifecycle callback can be class methods.
2. Creates a component implementation using a unique_ptr. 
3. Create a C++ DM Component and directly add it to the DM. For C++ DM Component needs to be "build" first, before the
   DM will activate them. This way C++ components can be build using a fluent api and marked ready with a `build()`
   method call. 
   As component implementation the DM accepts a unique_pt, shared_ptr, value type of no implementation. If no 
   implementation is provided the DM will create a component implementation using the template argument and 
   assuming a default constructor (e.g. `ctx->getDependencyManager()->createComponent<CmpWithDefaultCTOR>()`). 
4. Configures the component lifecycle callbacks as class methods. The DM will call these callbacks using the
   component implementation as object instance.
5. "Builds" the component. C++ component will only be managed by the DM after they are build. This makes it possible
   to configure a component over multiple method calls before marking the component as ready (build).
   The generated C++ bundle activator will also enable all components created during the bundle activation, to ensure  
   that the build behaviour is backwards compatible with previous released DM implementation. It is preferred that
   users explicitly build their components when they are completely configured.

```C++
//src/SimpleComponentActivator.cc
#include <celix/BundleActivator.h>

class SimpleComponent {
public:
    void init() { // <-----------------------------------------------------------------------------------------------<1>
        std::cout << "Initializing simple component. Transition nr " << transitionCount++ << std::endl;
    }

    void start() {
        std::cout << "starting simple component. Transition nr " << transitionCount++ << std::endl;
    }

    void stop() {
        std::cout << "Stopping simple component. Transition nr " << transitionCount++ << std::endl;
    }

    void deinit() {
        std::cout << "De-initializing simple component. Transition nr " << transitionCount++ << std::endl;
    }
private:
    int transitionCount = 1; //not protected, only updated and read in the celix event thread.
};

class SimpleComponentActivator {
public:
    explicit SimpleComponentActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto cmp = std::make_unique<SimpleComponent>(); // <---------------------------------------------------------<2>
        ctx->getDependencyManager()->createComponent(std::move(cmp), "SimpleComponent1") // <------------------------<3>
                .setCallbacks(
                        &SimpleComponent::init,
                        &SimpleComponent::start,
                        &SimpleComponent::stop,
                        &SimpleComponent::deinit) // <---------------------------------------------------------------<4>
                .build(); // <---------------------------------------------------------------------------------------<5>
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(SimpleComponentActivator)
```

# Component's Provided Services
Components can be configured to provide services. These provided services will correspondent to service registration 
when a component is in the `Tracking Optional` state. 

If a component provide services, these service will have an additional metadata property - named "component.uuid"
that couples the services to the component named "component.uuid".

## Example: Component with a provided service in C
The following example shows how a component that provide a `celix_shell_command` service. 

Remarks for the C example:
1. C and C services do not support inheritance. So even if a C component provides a certain service it is not an
   instance of said service. This also means the C service struct provided by a component needs to be stored 
   separately. In this example this is done storing the service struct in the bundle activator data. Note
   that the bundle activator data "outlives" the component, because all components are removed before a bundle 
   is completely stopped.
2. Configures a provided service (interface) for the component. The service will not directly be registered, but
   instead will be registered if the component enters the state `Tracking Optional`.

```C
 //src/component_with_provided_service_activator.c
#include <stdlib.h>
#include <celix_api.h>
#include <celix_shell_command.h>

//********************* COMPONENT *******************************/

typedef struct component_with_provided_service {
    int callCount; //atomic
} component_with_provided_service_t;

static component_with_provided_service_t* componentWithProvidedService_create() {
    component_with_provided_service_t* cmp = calloc(1, sizeof(*cmp));
    return cmp;
}

static void componentWithProvidedService_destroy(component_with_provided_service_t* cmp) {
    free(cmp);
}

static bool componentWithProvidedService_executeCommand(
        component_with_provided_service_t *cmp,
        const char *commandLine,
        FILE *outStream,
        FILE *errorStream __attribute__((unused))) {
    int count = __atomic_add_fetch(&cmp->callCount, 1, __ATOMIC_SEQ_CST);
    fprintf(outStream, "Hello from cmp. command called %i times. commandLine: %s\n", count, commandLine);
    return true;
}

//********************* ACTIVATOR *******************************/

typedef struct component_with_provided_service_activator {
    celix_shell_command_t shellCmd; // <-----------------------------------------------------------------------------<1>
} component_with_provided_service_activator_t;

static celix_status_t componentWithProvidedServiceActivator_start(component_with_provided_service_activator_t *act, celix_bundle_context_t *ctx) {
    //creating component
    component_with_provided_service_t* impl = componentWithProvidedService_create();

    //create and configuring component and its lifecycle callbacks using the Apache Celix Dependency Manager
    celix_dm_component_t* dmCmp = celix_dmComponent_create(ctx, "component_with_provided_service_1");
    celix_dmComponent_setImplementation(dmCmp, impl);
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(
            dmCmp,
            component_with_provided_service_t,
            componentWithProvidedService_destroy);

    //configure provided service
    act->shellCmd.handle = impl;
    act->shellCmd.executeCommand = (void*)componentWithProvidedService_executeCommand;
    celix_properties_t* props = celix_properties_create();
    celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, "hello_component");
    celix_dmComponent_addInterface(
            dmCmp,
            CELIX_SHELL_COMMAND_SERVICE_NAME,
            CELIX_SHELL_COMMAND_SERVICE_VERSION,
            &act->shellCmd,
            props); // <---------------------------------------------------------------------------------------------<2>


    //Add dm component to the dm.
    celix_dependency_manager_t* dm = celix_bundleContext_getDependencyManager(ctx);
    celix_dependencyManager_add(dm, dmCmp);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(
        component_with_provided_service_activator_t,
        componentWithProvidedServiceActivator_start,
        NULL)
```

## Example: Component with a provided service in C++
The following example shows how a C++ component that provide a C++ `celix::IShellCommand` service 
and a C `elix_shell_command` service. For a C++ component it's possible to provide C and C++ services.  

Remarks for the C++ example:
1. If a component provides a C++ services, it also expected that the component implementation inherits the service
   interface. 
2. The overridden `executeCommand` method of `celix::IShellCommand`.
3. Methods of C service interfaces can be implemented as class methods, but the bundle activator should ensure that 
   the underlining C service interface structs are assigned with compatible C function pointers. 
4. Creating a component using only a template argument. The DM will construct - using a default constructor - a 
   component implementation instance.
5. Configures the component to provide a C++ `celix::IShellCommand` service. Note that because the component
   implementation is an instance of `celix::IShellCommand` no additional storage is needed. The service will not 
   directly be registered, but instead will be registered if the component enters the state `Tracking Optional`.
6. Set the C `executeCommand` function pointer of the `celix_shell_command_t` service interface struct to a 
   capture-less lambda expression. The lambda expression is used to forward the call to the `executeCCommand` 
   class method. Note the capture-less lambda expression can decay to function pointers. 
7. Configures the component to provide a C `celix_shell_command_t` service. Note that for a C service, the service
   interface struct also needs to be stored to ensure that the service interface struct will outlive the component.
   The service will not directly be registered, but instead will be registered if the component enters the state
   `Tracking Optional`..
8. "Build" the component so the DM will manage and try to activate the component. 


```C++
//src/ComponentWithProvidedServiceActivator.cc
#include <celix/BundleActivator.h>
#include <celix/IShellCommand.h>
#include <celix_shell_command.h>

class ComponentWithProvidedService : public celix::IShellCommand { // <----------------------------------------------<1>
public:
    ~ComponentWithProvidedService() noexcept override = default;

    void executeCommand(
            const std::string& commandLine,
            const std::vector<std::string>& /*commandArgs*/,
            FILE* outStream,
            FILE* /*errorStream*/) override {
        fprintf(outStream, "Hello from cmp. C++ command called %i times. commandLine is %s\n", cxxCallCount++, commandLine.c_str());
    } // <-----------------------------------------------------------------------------------------------------------<2>

    void executeCCommand(const char* commandLine, FILE* outStream) {
        fprintf(outStream, "Hello from cmp. C command called %i times. commandLine is %s\n", cCallCount++, commandLine);
    } // <-----------------------------------------------------------------------------------------------------------<3>
private:
    std::atomic<int> cxxCallCount{1};
    std::atomic<int> cCallCount{1};
};

class ComponentWithProvidedServiceActivator {
public:
    explicit ComponentWithProvidedServiceActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto& cmp = ctx->getDependencyManager()->createComponent<ComponentWithProvidedService>(); // <---------------<4>

        cmp.createProvidedService<celix::IShellCommand>()
            .addProperty(celix::IShellCommand::COMMAND_NAME, "HelloComponent"); // <---------------------------------<5>

        shellCmd.handle = static_cast<void*>(&cmp.getInstance());
        shellCmd.executeCommand = [](void* handle, const char* commandLine, FILE* outStream, FILE*) -> bool {
            auto* impl = static_cast<ComponentWithProvidedService*>(handle);
            impl->executeCCommand(commandLine, outStream);
            return true;
        }; // <------------------------------------------------------------------------------------------------------<6>
        cmp.createProvidedCService(&shellCmd, CELIX_SHELL_COMMAND_SERVICE_NAME)
            .addProperty(CELIX_SHELL_COMMAND_NAME, "hello_component"); // < -----------------------------------------<7>

        cmp.build(); // <--------------------------------------------------------------------------------------------<8>
    }
private:
    celix_shell_command_t shellCmd{nullptr, nullptr};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(ComponentWithProvidedServiceActivator)
```

# Component's Service Dependencies
Components can be configured to have service dependencies. These service dependencies will influence the component's 
lifecycle. Components can have optional and required service dependencies. When service dependencies are required the
component can only be active if all required dependencies are available; 

When configuring service dependencies, callbacks can be configured when services are being added, removed or when a 
new highest ranking service is available. It is also possible to configure callbacks which only inject/remove service 
pointers, or service pointers with their properties (metadata) and even service pointers with their properties and
their providing bundle. Service dependency callbacks will always be called from the Celix event thread.

A service change (injection/removal) can be handled by the component using a Locking-strategy or a suspend-strategy.
This strategy can be configured per service dependency and expect the following behaviour from the component 
implementation:
- Locking-strategy: The component implementation must ensure that the service pointers (and if applicable the service
  properties and its bundle) are protected using a locking mechanism (e.g. a mutex). This should ensure that services
  are no longer in use after they are removed (or replaced) from a component. 
- Suspend-strategy: The DM will ensure that before service dependency callbacks are called, all provided services
  are (temporary) unregistered and the component is suspended (using the components' stop callback). This should mean
  that there are no active users - through the provided services or active threads - of the service dependencies 
  anymore and that service changes can safely be handling without locking. The component implementation must ensure
  that after `stop` callback no active thread, thread pools, timers, etc that use service dependencies are active 
  anymore.

## Example: Component with a service dependencies in C
The following example shows how a C component that has two service dependency on the `celix_shell_command_t` service.

One service dependency is a required dependency with a suspend-strategy and uses a `set ` callback which ensure 
that a single service is injected and that is always the highest ranking service. Note that the highest ranking
service can be `NULL` if there are no service. 

The other dependency is an optional dependency with a locking-strategy and uses a `addWithProps` and 
`removeWithProps` callback. These callbacks will be called for every `celix_shell_command_t` service being added/removed
and will be called with not only the service pointer, but also the service metadata properties. 

Remarks for the C example:
1. TODO

```C
//src/component_with_service_dependency_activator.c
#include <stdlib.h>
#include <celix_api.h>
#include <celix_shell_command.h>

//********************* COMPONENT *******************************/

typedef struct component_with_service_dependency {
    celix_shell_command_t* highestRankingCmdShell; //only updated when component is not active or suspended
    celix_thread_mutex_t mutex;
    celix_array_list_t* cmdShells;
} component_with_service_dependency_t;

static component_with_service_dependency_t* componentWithServiceDependency_create() {
    component_with_service_dependency_t* cmp = calloc(1, sizeof(*cmp));
    celixThreadMutex_create(&cmp->mutex, NULL);
    cmp->cmdShells = celix_arrayList_create();
    return cmp;
}

static void componentWithServiceDependency_destroy(component_with_service_dependency_t* cmp) {
    celix_arrayList_destroy(cmp->cmdShells);
    celixThreadMutex_destroy(&cmp->mutex);
    free(cmp);
}

static void componentWithServiceDependency_setHighestRankingShellCommand(
        component_with_service_dependency_t* cmp,
        celix_shell_command_t* shellCmd) {
    printf("New highest ranking service (can be NULL): %p\n", shellCmd);
    cmp->highestRankingCmdShell = shellCmd;
}

static void componentWithServiceDependency_addShellCommand(
        component_with_service_dependency_t* cmp,
        celix_shell_command_t* shellCmd,
        const celix_properties_t* props) {
    long id = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    printf("Adding shell command service with service.id %li\n", id);
    celixThreadMutex_lock(&cmp->mutex);
    celix_arrayList_add(cmp->cmdShells, shellCmd);
    celixThreadMutex_unlock(&cmp->mutex);
}

static void componentWithServiceDependency_removeShellCommand(
        component_with_service_dependency_t* cmp,
        celix_shell_command_t* shellCmd,
        const celix_properties_t* props) {
    long id = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    printf("Removing shell command service with service.id %li\n", id);
    celixThreadMutex_lock(&cmp->mutex);
    celix_arrayList_remove(cmp->cmdShells, shellCmd);
    celixThreadMutex_unlock(&cmp->mutex);
}

//********************* ACTIVATOR *******************************/

typedef struct component_with_service_dependency_activator {
    //nop
} component_with_service_dependency_activator_t;

static celix_status_t componentWithServiceDependencyActivator_start(component_with_service_dependency_activator_t *act, celix_bundle_context_t *ctx) {
    //creating component
    component_with_service_dependency_t* impl = componentWithServiceDependency_create();

    //create and configuring component and its lifecycle callbacks using the Apache Celix Dependency Manager
    celix_dm_component_t* dmCmp = celix_dmComponent_create(ctx, "component_with_service_dependency_1");
    celix_dmComponent_setImplementation(dmCmp, impl);
    CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(
            dmCmp,
            component_with_service_dependency_t,
            componentWithServiceDependency_destroy);

    //create mandatory service dependency with cardinality one and with a suspend-strategy
    celix_dm_service_dependency_t* dep1 = celix_dmServiceDependency_create();
    celix_dmServiceDependency_setService(dep1, CELIX_SHELL_COMMAND_SERVICE_NAME, NULL, NULL);
    celix_dmServiceDependency_setStrategy(dep1, DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND);
    celix_dmServiceDependency_setRequired(dep1, true);
    celix_dm_service_dependency_callback_options_t opts1 = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
    opts1.set = (void*)componentWithServiceDependency_setHighestRankingShellCommand;
    celix_dmServiceDependency_setCallbacksWithOptions(dep1, &opts1);
    celix_dmComponent_addServiceDependency(dmCmp, dep1);

    //create optional service dependency with cardinality many and with a locking-strategy
    celix_dm_service_dependency_t* dep2 = celix_dmServiceDependency_create();
    celix_dmServiceDependency_setService(dep2, CELIX_SHELL_COMMAND_SERVICE_NAME, NULL, NULL);
    celix_dmServiceDependency_setStrategy(dep2, DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);
    celix_dmServiceDependency_setRequired(dep2, false);
    celix_dm_service_dependency_callback_options_t opts2 = CELIX_EMPTY_DM_SERVICE_DEPENDENCY_CALLBACK_OPTIONS;
    opts2.addWithProps = (void*)componentWithServiceDependency_addShellCommand;
    opts2.removeWithProps = (void*)componentWithServiceDependency_removeShellCommand;
    celix_dmServiceDependency_setCallbacksWithOptions(dep2, &opts2);
    celix_dmComponent_addServiceDependency(dmCmp, dep2);

    //Add dm component to the dm.
    celix_dependency_manager_t* dm = celix_bundleContext_getDependencyManager(ctx);
    celix_dependencyManager_add(dm, dmCmp);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(
        component_with_service_dependency_activator_t,
        componentWithServiceDependencyActivator_start,
        NULL)
```

## Example: Component with a service dependencies in C++
The following example shows how a C++ component that has two service dependency. One 
service dependency for the C++ `celix::IShellCommand` service and one for the C `celix_shell_command_t` service.

The `celix::IShellCommand` service dependency is a required dependency with a suspend-strategy and uses a 
`set ` callback which ensure that a single service is injected and that is always the highest ranking service. 
Note that the highest ranking service can be an empty shared_ptr if there are no service.

The `celix_shell_command_t` service dependency is an optional dependency with a locking-strategy and uses a
`addWithProperties` and `removeWithProperties` callback. 
These callbacks will be called for every `celix_shell_command_t` service being added/removed
and will be called with not only the service shared_ptr, but also the service metadata properties.

Note that for C++ component service dependencies there is no real different between a C++ or a C service; Both
are injected as shared_ptr, and as optional properties or bundle argument the C++ variant are provided.

Remarks for the C++ example:
1. TODO

```C++
//src/ComponentWithServiceDependencyActivator.cc
#include <celix/BundleActivator.h>
#include <celix/IShellCommand.h>
#include <celix_shell_command.h>

class ComponentWithServiceDependency {
public:
    void setHighestRankingShellCommand(const std::shared_ptr<celix::IShellCommand>& cmdSvc) {
        std::cout << "New highest ranking service (can be NULL): " << (intptr_t)cmdSvc.get() << std::endl;
        highestRankingShellCmd = cmdSvc;
    }

    void addCShellCmd(
            const std::shared_ptr<celix_shell_command_t>& cmdSvc,
            const std::shared_ptr<const celix::Properties>& props) {
        auto id = props->getAsLong(celix::SERVICE_ID, -1);
        std::cout << "Adding shell command service with service.id: " << id << std::endl;
        std::lock_guard lck{mutex};
        shellCommands.emplace(id, cmdSvc);
    }

    void removeCShellCmd(
            const std::shared_ptr<celix_shell_command_t>& /*cmdSvc*/,
            const std::shared_ptr<const celix::Properties>& props) {
        auto id = props->getAsLong(celix::SERVICE_ID, -1);
        std::cout << "Removing shell command service with service.id: " << id << std::endl;
        std::lock_guard lck{mutex};
        shellCommands.erase(id);
    }
private:
    std::shared_ptr<celix::IShellCommand> highestRankingShellCmd{};
    std::mutex mutex{}; //protect below
    std::unordered_map<long, std::shared_ptr<celix_shell_command_t>> shellCommands{};
};

class ComponentWithServiceDependencyActivator {
public:
    explicit ComponentWithServiceDependencyActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        using Cmp = ComponentWithServiceDependency;
        auto& cmp = ctx->getDependencyManager()->createComponent<Cmp>(); // <-------------<1>

        cmp.createServiceDependency<celix::IShellCommand>()
                .setCallbacks(&Cmp::setHighestRankingShellCommand)
                .setRequired(true)
                .setStrategy(DependencyUpdateStrategy::suspend);

        cmp.createServiceDependency<celix_shell_command_t>(CELIX_SHELL_COMMAND_SERVICE_NAME)
                .setCallbacks(&Cmp::addCShellCmd, &Cmp::removeCShellCmd)
                .setRequired(false)
                .setStrategy(DependencyUpdateStrategy::locking);

        cmp.build(); // <--------------------------------------------------------------------------------------------<8>
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(ComponentWithServiceDependencyActivator)
```

# When will a component be suspended

TODO explain that a component will be suspended with service dependency on suspend strategy, a matching service update 
and if there is a svc inject callback configured. TODO check also start/stop method?
