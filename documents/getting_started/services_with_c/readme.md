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

# Apache Celix - Using Services with C

## Intro 

This example gives an overview for providing and using services with Apache Celix with C.

## Services

To start of, C services in Celix are just a pointer to a memory location registered in the service registry using a name and an optional set of key/value pairs. 

By convention use the following service layout:

```C
//example.h
#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#define EXAMPLE_NAME            "org.example"
#define EXAMPLE_VERSION         "1.0.0"
#define EXAMPLE_CONSUMER_RANGE  "[1.0.0,2.0.0)"


struct example_struct {
	void *handle;
	int (*method)(void *handle, int arg1, double arg2, double *result);
} ;

typedef struct example_struct example_t;

#endif /* EXAMPLE_H_ */

```


For a Celix service a service name, service provider version and service consumer range should be declared.
This is explicitly done with macros to prevent symbols so to that no linking dependencies are introduced.

Then the actual struct for the service needs to be declared.
The first element of the service struct should be a handle which can be used to store the service context, as convention we keep this pointer a void pointer to explicitly make it opaque.
Note that also an opaque struct could be used (e.g a declared but not defined struct), but this can become problematic concerning components registering multiple services. 
In that case explicit cast are needed to prevent warning and this can be confusing for the To prevent that issues void pointers are preferred.

The rest of the element should be function pointers, which by convention should return an celix_status_t or int (which is technically the same). 
The return value is used as a way of handling errors and is also needed to be able to make remote services (e.g. to be able to handle remote exceptions).

The first argument of a service function should be the service handle and if there is a result the last argument should be a output parameter (either pre allocated (e.g. double *) or not (e.g. double **)).
If the caller is not the owner of the output argument, a const pointer should be used (e.g. const char**). 
It is also possible to create typedef of the pointer to the service struct (e.g. typedef struct example_struct example_t), but this is not needed. 

In the Celix code base there are still service which uses a typedef with a pointer (e.g. typedef struct example_struct* example_struct_pt). This should be avoided, 
because it is not possible to create the const pointer of those typedefs and it is not possible to include those typedef inside a existing struct without the needed for an additional malloc.

### Semantic Versioning

For versioning, semantic versioning should be used.

A backward incompatible change should lead to a major version increase (e.g. 1.0.0 -> 2.0.0).
For a C Service versioning is used to express binary compatibility (for the same platform / compiler), change that are incompatible are:

- Removing a function
- Adding a function to before any other function
- Moving a function to an other location in the service struct
- Changing the signature of a function
- Changing the semantics of a argument (e.g. changing range input from "range in kilometer" to "range in meters")

A backwards binary compatible change which extend the functionality should lead to a minor version increase (e.g. 1.0.0 -> 1.1.0).
Changes considered backwards compatible which extend the functionality are:

- Adding a function to the back of the service struct

A backwards binary compatible change which does not extend the functionality should lead to a micro version increase (e.g. 1.0.0 -> 1.0.1).
Changes considered backwards binary compatible which does not extend the functionality are:

- Changes in the documentation
- Renaming of arguments

For C services generally platform specific calling convention are used therefore binary compatibility between service provider and consumers from different compilers is possible (e.g. gcc and clang), 
 but not advisable

 
## Components

Component should use the ADT principle (see [ADT in C](http://inst.eecs.berkeley.edu/~selfpace/studyguide/9C.sg/Output/ADTs.in.C.html)).
Note that is a convention.

Components should have a `<cmpName>_create` and `<cmpName>_destroy` function.
Components can have a `<cmpName>_start` and `<cmpName>_stop` function to start/stop threads or invoke functionality needed a fully created component. 
The start function will only be called if all required service are available and the stop function will be called when some required are going or if the component needs to be stopped.

Components can also have a `<cmpName>_init` and `<cmpName>_deinit` function which will be called before and after respectively the start and stop function. 
The init/deinit function can be used to include (de)initialization which is not needed/wanted every time when service dependencies are being removed/added. 

## Code Examples

The next code blocks contains some code examples of components to indicate how to handle service dependencies, how to specify providing services and how to cope with locking/synchronizing.
The complete example can be found [here](../../examples/services_example_c).

The error checking is very minimal in these example to keep the focus on how to interact with services and how to deal with errors in C / Celix.


### Bar example

The bar example is a simple component providing the `example` service. 
 
```C
//bar.h
#ifndef BAR_H_
#define BAR_H_

#include "example.h"

typedef struct bar_struct bar_t;

bar_t* bar_create(void);
void bar_destroy(bar_t *self);

int bar_method(bar_t *self, int arg1, double arg2, double *out);

#endif //BAR_H_
```

```C
//bar.c
#define OK 0
#define ERROR 1

struct bar_struct {
    double prefValue;
};

bar_t* bar_create(void) {
    bar_t *self = calloc(1, sizeof(*self));
    if (self != NULL) {
        self->prefValue = 42;
    } else {
        //log error
    }
    return self;
};

void bar_destroy(bar_t *self) {
    free(self);
}

int bar_method(bar_t *self, int arg1, double arg2, double *out) {
    double update = (self->prefValue + arg1) * arg2;
    self->prefValue = update;
    *out = update;
    return OK;
}
```

```C
//bar_activator.c
#include "dm_activator.h"
#include "bar.h"

#include <stdlib.h>

struct activator {
	bar_t *bar;
	example_t exampleService;
};

celix_status_t dm_create(bundle_context_pt context, void **userData) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *act = calloc(1, sizeof(*act));
    if (act != NULL) {

        act->bar = bar_create();
        act->exampleService.handle = act->bar;
        act->exampleService.method = (void*) bar_method;

        if (act->bar != NULL) {
            *userData = act;
        } else {
            free(act);
        }
    } else {
        status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t dm_init(void *userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *activator = userData;

    dm_component_pt cmp = NULL;
    component_create(context, "BAR", &cmp);
    component_setImplementation(cmp, activator->bar);
    component_addInterface(cmp, EXAMPLE_NAME, EXAMPLE_VERSION, &activator->exampleService, NULL);

    dependencyManager_add(manager, cmp);
    return status;
}

celix_status_t dm_destroy(void *userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *activator = userData;
    bar_destroy(activator->bar);
    free(activator);
    return status;
};
```

### Foo1 example

The Foo1 example shows how add a service dependency, implement the callback, invoke a service and how to protect the usage of service with use of a mutex.

```C
//foo1.h
#ifndef FOO1_H_
#define FOO1_H_

#include "example.h"

typedef struct foo1_struct foo1_t;

foo1_t* foo1_create(void);
void foo1_destroy(foo1_t *self);

int foo1_start(foo1_t *self);
int foo1_stop(foo1_t *self);

int foo1_setExample(foo1_t *self, const example_t *example);


#endif //FOO1_H_
```

```C
//foo1.c
#include "foo1.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>


#define OK 0
#define ERROR 1

static void* foo1_thread(void*);

struct foo1_struct {
    const example_t *example;
    pthread_mutex_t mutex; //protecting example
    pthread_t thread;
    bool running;
};

foo1_t* foo1_create(void) {
    foo1_t *self = calloc(1, sizeof(*self));
    if (self != NULL) {
        pthread_mutex_init(&self->mutex, NULL);
        self->running = false;
    } else {
        //log error
    }
    return self;
};

void foo1_destroy(foo1_t *self) {
    assert(!self->running);
    pthread_mutex_destroy(&self->mutex);
    free(self);
}

int foo1_start(foo1_t *self) {
    self->running = true;
    pthread_create(&self->thread, NULL, foo1_thread, self);
    return OK;
}

int foo1_stop(foo1_t *self) {
    self->running = false;
    pthread_kill(self->thread, SIGUSR1);
    pthread_join(self->thread, NULL);
    return OK;
}

int foo1_setExample(foo1_t *self, const example_t *example) {
    pthread_mutex_lock(&self->mutex);
    self->example = example; //NOTE could be NULL if req is not mandatory
    pthread_mutex_unlock(&self->mutex);
    return OK;
}

static void* foo1_thread(void *userdata) {
    foo1_t *self = userdata;
    double result;
    int rc;
    while (self->running) {
        pthread_mutex_lock(&self->mutex);
        if (self->example != NULL) {
            rc = self->example->method(self->example->handle, 1, 2.0, &result);
            if (rc == 0) {
                printf("Result is %f\n", result);
            } else {
                printf("Error invoking method for example\n");
            }
        }
        pthread_mutex_unlock(&self->mutex);
        usleep(10000000);
    }
    return NULL;
}
```

```C
//foo1_activator.c
#include "dm_activator.h"
#include "foo1.h"

#include <stdlib.h>

struct activator {
	foo1_t *foo;
};

celix_status_t dm_create(bundle_context_pt context, void **userData) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *act = calloc(1, sizeof(*act));
    if (act != NULL) {
        act->foo = foo1_create();
        if (act->foo != NULL) {
            *userData = act;
        } else {
            free(act);
        }
    } else {
        status = CELIX_ENOMEM;
    }
    return status;
}

celix_status_t dm_init(void *userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *activator = userData;

    dm_component_pt cmp = NULL;
    component_create(context, "FOO1", &cmp);
    component_setImplementation(cmp, activator->foo);

    /*
    With the component_setCallbacksSafe we register callbacks when a component is started / stopped using a component
     with type foo1_t*
    */
    component_setCallbacksSafe(cmp, foo1_t*, NULL, foo1_start, foo1_stop, NULL);

    dm_service_dependency_pt dep = NULL;
    serviceDependency_create(&dep);
    serviceDependency_setRequired(dep, true);
    serviceDependency_setService(dep, EXAMPLE_NAME, EXAMPLE_CONSUMER_RANGE, NULL);
    serviceDependency_setStrategy(dep, DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);

    /*
    With the serviceDependency_setCallbacksSafe we register callbacks when a service
    is added and about to be removed for the component type foo1_t* and service type example_t*.

    We should protect the usage of the
    service because after removal of the service the memory location of that service
    could be freed
    */
    serviceDependency_setCallbacksSafe(dep, foo1_t*, const example_t*, foo1_setExample, NULL, NULL, NULL, NULL);
    component_addServiceDependency(cmp, dep);

    dependencyManager_add(manager, cmp);

    return status;
}

celix_status_t dm_destroy(void *userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	foo1_destroy(activator->foo);
	free(activator);
	return status;
};

```

### Foo2 example

The Foo2 example shows how to cope with multiple services and how to remove the need for locking by ensuring only access to the services and the services container by a single thread.

```C
//foo2.h
#ifndef FOO2_H_
#define FOO2_H_

#include "example.h"

typedef struct foo2_struct foo2_t;

foo2_t* foo2_create(void);
void foo2_destroy(foo2_t *self);

int foo2_start(foo2_t *self);
int foo2_stop(foo2_t *self);

int foo2_addExample(foo2_t *self, const example_t *example);
int foo2_removeExample(foo2_t *self, const example_t *example);

#endif //FOO2_H_
```

```C
//foo2.c
#include "foo2.h"

#include "array_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>


#define OK 0
#define ERROR 1

static void* foo2_thread(void*);

struct foo2_struct {
    array_list_pt examples;
    pthread_t thread;
    bool running;
};

foo2_t* foo2_create(void) {
    foo2_t *self = calloc(1, sizeof(*self));
    if (self != NULL) {
        self->examples = NULL;
        arrayList_create(&self->examples);
        self->running = false;
    } else {
        //log error
    }
    return self;
};

void foo2_destroy(foo2_t *self) {
    assert(!self->running);
    arrayList_destroy(self->examples);
    free(self);
}

int foo2_start(foo2_t *self) {
    self->running = true;
    pthread_create(&self->thread, NULL, foo2_thread, self);
    return OK;
}

int foo2_stop(foo2_t *self) {
    self->running = false;
    pthread_kill(self->thread, SIGUSR1);
    pthread_join(self->thread, NULL);
    return OK;
}

int foo2_addExample(foo2_t *self, const example_t *example) {
    //NOTE foo2 is suspended -> thread is not running  -> safe to update
    int status = OK;
    status = arrayList_add(self->examples, (void *)example);
    return status;
}

int foo2_removeExample(foo2_t *self, const example_t *example) {
    //NOTE foo2 is suspended -> thread is not running  -> safe to update
    int status = OK;
    status = arrayList_removeElement(self->examples, (void*)example);
    return status;
}

static void* foo2_thread(void *userdata) {
    foo2_t *self = userdata;
    double result;
    int rc;
    while (self->running) {
        unsigned int size = arrayList_size(self->examples);
        int i;
        for (i = 0; i < size; i += 1) {
            const example_t* example = arrayList_get(self->examples, i);
            rc = example->method(example->handle, 1, 2.0, &result);
            if (rc == 0) {
                printf("Result is %f\n", result);
            } else {
                printf("Error invoking method for example\n");
            }
        }
        usleep(10000000);
    }
    return NULL;

```

```C
//foo2_activator.c
#include "dm_activator.h"
#include "foo2.h"

#include <stdlib.h>

struct activator {
	foo2_t *foo;
};

celix_status_t dm_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *act = calloc(1, sizeof(*act));
	if (act != NULL) {
		act->foo = foo2_create();
        if (act->foo != NULL) {
            *userData = act;
        } else {
            free(act);
        }
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t dm_init(void *userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
    celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	dm_component_pt cmp = NULL;
	component_create(context, "FOO2", &cmp);
	component_setImplementation(cmp, activator->foo);

	/*
	With the component_setCallbacksSafe we register callbacks when a component is started / stopped using a component
	 with type foo1_t*
	*/
    component_setCallbacksSafe(cmp, foo2_t*, NULL, foo2_start, foo2_stop, NULL);

	dm_service_dependency_pt dep = NULL;
	serviceDependency_create(&dep);
	serviceDependency_setRequired(dep, false);
	serviceDependency_setService(dep, EXAMPLE_NAME, EXAMPLE_CONSUMER_RANGE, NULL);
	serviceDependency_setStrategy(dep, DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND);

	/*
	With the serviceDependency_setCallbacksSafe we register callbacks when a service
	is added and about to be removed for the component type foo1_t* and service type example_t*.

	We should protect the usage of the
 	service because after removal of the service the memory location of that service
	could be freed
	*/
    serviceDependency_setCallbacksSafe(dep, foo2_t*, const example_t*, NULL, foo2_addExample, NULL, foo2_removeExample, NULL);
	component_addServiceDependency(cmp, dep);

	dependencyManager_add(manager, cmp);

    return status;
}

celix_status_t dm_destroy(void *userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	foo2_destroy(activator->foo);
	free(activator);
	return status;
};
```

## Locking and Suspending
 
As you may notice, the Foo1 example uses locks. 
In principle, locking is necessary in order to ensure coherence in case service dependencies are removed/added/changed; on the other hands, locking increases latency and, when misused, can lead to poor performance. 
For this reason, the serviceDependency interface gives the possibility to choose between a locking and suspend (a non-locking) strategy through the serviceDependency_setStrategy function, as is used in the Foo2 example.

The locking strategy `DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING` notifies the component in case the dependencies' set changes (e.g. a dependency is added/removed): the component is responsible for protecting via locks the dependencies' list and check (always under lock) if the service he's depending on is still available.
The suspend or non-locking strategy `DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND` (default when no strategy is explicitly set) reliefs the programmer from dealing with service dependencies' consistency issues: in case this strategy is adopted, the component is stopped and restarted (i.e. temporarily suspended) upon service dependencies' changes.

The suspend strategy has the advantage of reducing locks' usage: of course, suspending the component has its own overhead (e.g. stopping and restarting threads), but this overhead is "paid" only in case of changes in service dependencies, while the locking overhead is always paid.

## See also

See the [C Dependeny Manager](../../libs/dependency_manager/readme.md) and [C Dependency Manager example](../../examples/celix-examples/dm_example) for more information and a more complex working example.
