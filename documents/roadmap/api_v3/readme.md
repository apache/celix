# Celix Version 3

## Intro
The [celix.h](celix/celix.h) header contains a proposed API for Apache Celix version 3.
This API is not fixed, but work in progress and should be considered a starting point for discussion.

There are a few reasons for a API change:

### The current Apache Celix API is mapped from the OSGi API.
The problem is that OSGi is designed for Java, a
language where classes are present runtime, reflection can be used, resource are controlled through garbage collection
and classes are kept available when still needed(garbage collector) even if a bundle is uninstalled.
This resulted in an API which is very lenient in object sharing/ownership and no real focus on resource
locking/protection. This simply does not map wel to a C language. For a dynamic service framework in C, the focus
should be in minimizing object sharing and when needed only for a controlled, well defined and short period. When
resource sharing is needed - i.e. services - projecting access to these resources should dictate the design of the
API.
In this proposed API, when possible, object sharing is done through sharing ids in the form of a primitive
(value copied) types instead of pointers. These ids can be used to
(safely) retrieve info form the object if still present, which should make it more safe to use and most importantly
less complex for implementation (think threads & locks). Also most service or service related updates must be
handled with callbacks, to ensure correct usage of locking can be applied more easily.

### OSGi API leaks too much details.
The OSGi API is more than 15 years old and has grown overtime, this shows.
There are a lot of hoops to take to use a service in a correct way.
The proposed Apache Celix API tries to minimize the needed objects, while still providing the same functionality.
It was also designed for a more user friendly experience.

### Smaller code base
Because the OSGi leaks to much details, it is also difficult to implemented; specifically mapped one to one from Java.
A redesign of the API could lead to a smaller code base.

### Single framework event thread design.
It would be beneficial to use single framework event thread,
this would means that as a user it is ensured that all the callbacks are called from the same thread.
Preventing issues that service can be added/removed simultaneously and the locking complexity which comes with that.
With the current API this is more difficult (too many entries to shared resources).

### Multiple languages
One of the key values of Apache Celix should be to deliver a sophisticated develop / design environment for more
complex - i.e. composed out of modules) application/systems. For example a applications with combines
some well proven C modules/libraries, with modern C++ modules and a splash of development support
with python modules.
And although Apache Celix is not there yet, it is one of it's goals.
The current API is not designed to support multiple languages and is difficult to port the different languages due to
the "too much details" issue.

### More focus on the primary ways to interact with a dynamic service framework.
For Apache Celix developers a focus is how to: work with services (register, tracking & monitor service interest) and
how to easily create components which provides and depends on services.
The proposed API focus on these issues as built-in support. For OSGi some of these functions where added add a later
stage and this shows.

### Current api has a strong focus on error codes
Java has exceptions, C has not. This has lead to an initial design that almost all function return a error code.
While in principle completely correct, from a developer perspective it become cumbersome.
Also taking in to perspective, than in many cases if an error is returned there no real way to handle this,
the proposed API focus more on easy of use and lenient and silently accepting invalid input (i.e. NULL pointers
or invalid ids).

### Integral API
Again because the OSGi API has grown overtime, the API seems to be dispersed and not one integral API.
The proposed API tries to remedy that.

### Runtime Type Introspection
With OSGi for Java you have runtime type introspection (reflection) build-in the Java language, for C
(and most other native languages) this is not the case. As effect is not possible to infer if services
are compatible or to automagically make service remote / serialize types.
The dynamic function interface library (dfi library) adds type introspection support, by using the extender pattern combined with so called type/interface descriptor files.
Although not ideal, this can be used to runtime check service compatibility and automagically serialize types.
If possible the dfi support will be optional and opt-out.

### Eat your own dog food
A bit strange, but as a service oriented framework the OSGi specification for the core framework specification does not provide framework services.
The proposed API moves some of the API for detailed info to framework services.

### Integrated event admin
Services are great, but not everything fits perfectly solely with services. The event admin from the OSGi
specification is a nice complement to the service oriented paradigm and as result event admin awareness is taken
into account in the proposed API.

### Missing prefix for symbols and headers
The current usage of prefixes is not sufficient. For example the celix framework has te be included as
"framework.h" and has no prefix. This is not wise for a language without namespaces. The proposed API
prefixes everything with celix_ and all include paths start with celix/.
TODO discuss if a more generic OSGi like prefix is desirable, e.g. nosgi_ and nosgi/

### const correctness
The proposed API add the use of const when applicable. The usage of const adds more semantics to the API and
can prevent certain calls (e.g.in the callbacks) when they should not be used.

### Support for a more static approach
Although Apache Celix is a framework for dynamic services, this does not mean that every module should be runtime install/uninstall-able.
Supporting static modules and modules as "plain old libraries" can make it easier to use and deploy application using Apache Celix.
In this proposal, bundle are still present, but just a way to install modules. Support for installing modules using static
 and shared libraries is added.

## Apache Celix V3 CMake Commands
Apache Celix provides several CMake commands to be able to work with Apache Celix modules, bundles and deployments.

### Generic Changes

- symbolic_name is no longer needed nor expected. A module_name / bundle_name
is enough and should be unique.
- All command are prefixed with `celix_`.

### Modules & Bundles

Modules are self-contained, autonomous and bounded software parts which can be combined to provide a greater/broader functionality.

Modules contain resources and required libraries (self-contained),
use a activator to bootstrap itself (autonomous),
can provide/require services to share/use functionality (bounded)
and can import / export libraries so share types / routines (bounded)

There are two types of modules
 - Library Modules
 - Bundled Modules (aka Bundles)

Library modules are singular modules which do not import nor export libraries,
cannot be bundled with additional required (private) libraries, and are build as shared or static libraries.
Library modules can contain additional (embedded) resources.
Static modules will only auto register on platforms where there is
support for the  `__attribute__((ctor))` compiler attribute.

Bundle modules are modules which can import and/or export other libraries and
can embed additional required (private) libraries.
Bundle modules will be build as bundle file (ZIP file).

### Modules

Modules can be build a "plain old libraries" using the CMake
add_library command. Resources can be added using the
celix_module_files command.

To be able to find the module resources, the module name used in the
celix_moduleRegister call should be the same as the module name used
in the CMake commnands.
If no module name is set using the celix_module_name CMake command,
the module name is equal to the name target.

```CMake

celix_module_name(<module_target> <module_name>)

celix_module_files(<bundle_or_module_target>
    files... DESTINATION <dir>
    [FILE_PERMISSIONS permissions...]
    [DIRECTORY_PERMISSIONS permissions...]
    [NO_SOURCE_PERMISSIONS] [USE_SOURCE_PERMISSIONS]
    [FILES_MATCHING]
    [[PATTERN <pattern> | REGEX <regex>]
    [EXCLUDE] [PERMISSIONS permissions...]] [...])
)
```

```C
/**
 * Registers the module for all current framework and future framework that are created.
 * If no name is provided, the module will not be registered and an error will be logged.
 *
 * When initializing the module, the framework will look for a <module_name>_resourceEntry and
 * <module_name>_resourceEntrySize symbol. If found, the content is assumed to be an embedded zip (or tar?) file.
 * the celix_modules_files CMake command will ensure that these symbols are created and linked to the module library.
 *
 * @param moduleName The name of the module.
 * @param moduleVersion Optional, the version of the module.
 * @startFp Optional the start function which will be called if the module is going to be added and ready to start.
 * @startFp Optional the stop function which will be called if the module going to be removed and ready to start.
 * @opts Optional the additional options. The options will only be used during the registerModule call.
 * @return 0 if successful
 */
int celix_moduleRegistration_registerModule(
        const char* moduleName,
        const char* moduleVersion,
        celix_module_registration_startModule_fp startFp,
        celix_module_registration_stopModule_fp stopFp,
        const celix_module_registration_options* opts);
```

### Bundles
Bundles are ZIP files which bundle one module.
The CMake commands for add_bundle commands are not changed with exception of the
added celix_ prefix. The other command are renamed to module and a celix_ is added
(e.g bundle_name -> celix_module_name)

```CMake
celix_add_bundle(<bundle_target_name>
    SOURCES source1 source2 ...
    [NAME bundle_name]
    [SYMBOLIC_NAME bundle_symbolic_name]
    [DESCRIPTION bundle_description]
    [VERSION bundle_version]
    [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
    [EXPORT_LIBRARIES export_lib1 export_lib2 ...]
    [IMPORT_LIBRARIES import_lib1 import_lib2 ...]
    [HEADERS "header1: header1_value" "header2: header2_value" ...]
)

celix_add_bundle(<bundle_target_name>
    ACTIVATOR <activator_lib>
    [NAME bundle_name]
    [SYMBOLIC_NAME bundle_symbolic_name]
    [DESCRIPTION bundle_description]
    [VERSION bundle_version]
    [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
    [EXPORT_LIBRARIES export_lib1 export_lib2 ...]
    [IMPORT_LIBRARIES import_lib1 import_lib2 ...]
    [HEADERS "header1: header1_value" "header2: header2_value" ...]
)

celix_add_bundle(<bundle_target_name>
    [NAME bundle_name]
    [SYMBOLIC_NAME bundle_symbolic_name]
    [DESCRIPTION bundle_description]
    [VERSION bundle_version]
    [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
    [EXPORT_LIBRARIES export_lib1 export_lib2 ...]
    [IMPORT_LIBRARIES import_lib1 import_lib2 ...]
    [HEADERS "header1: header1_value" "header2: header2_value" ...]
)

```

### Descriptors
Descriptors are files which describe interfaces or messages which can be
parsed by the dynamic function interface (dfi) library.
The dfi library can be used for runtime 'type introspection'.
The descriptor files are a bit cryptic for human eyes, but relatively easy to parse.

With these descriptors the Celix framework can runtime compare if
services and/or messages provider consumer combinations are compatible
and make remote services / serialization of messages possible.

The `celix_module_descriptor` CMake will register header files
for descriptor generation and inclusion in the module/bundle
resources (under META-INF/descriptors/interfaces and
META-INF/descriptors/messages).

The Celix project will have a `dfi-gen` target which will be used to
generate the descriptors from the header files.

```CMake

celix_module_descriptor(<bundle_or_module_target>
    header_file1 header_file2 ...
)
```

### Deployment

The `add_deploy` will be changed to `celix_add_deploy` and will accept
library modules and bundle modules.

```CMake
celix_add_deploy(<deploy_target_name>
    [COPY]
    [GROUP group_name]
    [NAME deploy_name]
    [LAUNCHER launcher]
    [DIR dir]
    [MODULES <module_or_bundle1> <module_or_bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
)
```
