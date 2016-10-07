#Apache Celix - Using Services with C

## Intro 

This example gives an overview for providing and using C and C++ services with Apache Celix with C++.

## Services

### C++ Services
TO start of, C++ service in Celix are just (abstract) classes. 

In the following example there also a projected default constructor to ensure no instantiation of the service is possible:
```C++
//IAnotherExample.h
#ifndef IANOTHER_EXAMPLE_H
#define IANOTHER_EXAMPLE_H

#define IANOTHER_EXAMPLE_VERSION "1.0.0"
#define IANOTHER_EXAMPLE_CONSUMER_RANGE "[1.0.0,2.0.0)"

class IAnotherExample {
protected:
    IAnotherExample() = default;
public:
    virtual double method(int arg1, double arg2) = 0;
};

#endif //IANOTHER_EXAMPLE_H
```

For a Celix service a service name, service provider version and service consumer range should be declared.
This is explicitly done with macros to prevent symbols so to that no linking dependencies are introduced. 
For C++ the service name can be inferred. 

### C Services
C services in Celix are just a pointer to a memory location registered in the service registry using a name and an optional set of key/value pairs.

By convention use the following service layout:
```C
//example.h
#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define EXAMPLE_NAME 			"org.example"
#define EXAMPLE_VERSION 		"1.0.0"
#define EXAMPLE_CONSUMER_RANGE   "[1.0.0,2.0.0)"


struct example_struct {
	void *handle;
	int (*method)(void *handle, int arg1, double arg2, double *result);
} ;

typedef struct example_struct example_t;

#ifdef __cplusplus
}
#endif

#endif /* EXAMPLE_H_ */

```

For C service a struct containing the function pointers needs to be declared.
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

### Versioning C++ Services
For C++ Services versioning is used ot express binary compatibility changes that are incompatible are:

- Everything. Seriously, binary compatibility in C++ is difficult and should be avoided. 

Note that is is possible to use versioning for source compatibility and setup the build environment accordingly, but this is not part of this guide.

### Versioning C Services
For C Services versioning is used to express binary compatibility (for the same platform / compiler), change that are incompatible are:

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

Component are concrete classes in C++. This do not have to implement specific interface, expect the C++ service interfaces they provide.

## Code Examples

The next code blocks contains some code examples of components to indicate how to handle service dependencies, how to specify providing services and how to cope with locking/synchronizing.
The complete example can be found [here](../../examples/services_example_cxx).

### Bar example

The bar example is a simple component providing the C `example` service and C++ `IAnotherExample` service. 


TODO

## See also

See the [C++ Dependency Manager example](../../examples/dm_example_cxx) for a working example and good starting point to create more complex bundles.