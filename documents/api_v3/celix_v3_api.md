# Celix Version 3

## Intro
The [celix.h](api_v3/celix/celix.h) header contains a proposed API for Apache Celix version 3.
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
The proposed Apache Celix API tries to minimize the needed objects, while still provided the same functionality.
It was also design for a more user friendly experience, taking into account that dynamic service can be daunting.

### Smaller code base
Because the OSGi leaks to much details, it is also difficult to implemented; specifically mapped one to one from Java.
A redesign of the API could lead to a smaller code base.

### Single framework event thread design.
It would be beneficial to use single framework event thread, this would means that as a user it is ensured that the
all the callbacks are called from the same thread. Preventing issues that service can be added/removed simultaneously
and the locking complexity which comes with that.
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
of invalid ids).

### Eat your own dog food
A bit strange, but as a service oriented framework,
the OSGi core framework specification does not provide services on itself.
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
can prevent certain calls (e.g.in the callbacks) if when they should not be used.

### Support for a more static approach
Although Apache Celix is a framework for dynamic services, this does not mean that every module should be runtime install/uninstall able.
Supporting static modules and modules as "plain old libraries" can make it easier to use and deploy application using Apache Celix.
In this proposal, bundle are still present, but just a way to install modules. Support for installing modules using static
 and shared libraries is also supported.

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

celix_module_files(<module_target>
    files... DESTINATION <dir>
    [FILE_PERMISSIONS permissions...]
    [DIRECTORY_PERMISSIONS permissions...]
    [NO_SOURCE_PERMISSIONS] [USE_SOURCE_PERMISSIONS]
    [FILES_MATCHING]
    [[PATTERN <pattern> | REGEX <regex>]
    [EXCLUDE] [PERMISSIONS permissions...]] [...])
)
```

### Bundles
Bundles are ZIP files which bundle one module.
The CMake commands for bundle is not changed with expection of the
added celix_ prefix.

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

### Deployment

TODO update deployment to be able to select bundles and modules
