# Improvement Ideas


# Extend Dependency Manager C/C++
The dependency manager offers declarative API for working with services and is arguable more easier 
to use than the official OSGi Api. Extend the DM so that in can be used for virtually any bundle 
without directly using the bundle context, bundle, module, service ref, service registration, etc API.
This provided a much smaller API providing almost the same functionality.

This means:
1. Add support for service factory 
1. Add support for providing listener hooks. Note that this is already supported by registering 
   a `listener_hook_service` service, but maybe a more abstracted way.
1. Add support looking up and opening resources in the bundle.
1. Replace add,remove, etc callbacks which uses the service reference, with callbacks using properties
1. Test if the dm correctly works when adding / removing provided services and service dependencies after a component is started.
1. Add support for bundle listener without needing the bundle.h API
1. Add support for listing installed bundles without needing the bundle.h/bundle_context.h API
1. Only required dependencies should be in celix_utils. This means: properties, lists, celix errors. 
 
# Introduce dlmopen for library imports.
Currently library are loaded LOCAL for bundles. This works alright, but makes it hard to add a concept 
of exporting and importing libraries. 

The trick is that the NEEDED header in the importing libraries 
should match the target exported library SONAME header and no other exported libraries SONAME headers. 
One solution to make this work is to alter the NEEDED & SONAME runtime. 
 
For glibc there is now an other alternative, namely using dlmopen. dlmopen makes it possible to load 
libraries in a given namespace. For bundle this can be used to create a library namespace per bundle 
and load exported libraries int importing bundle namespace. A clean solution, but this currently 
only works for glibc (linux).

# Refactor service registry to a single threaded design
Celix currently has some nested lock. It would be preferable to remove these, but this is difficult 
because they are used to sync events which react on registering/unregistering services. Specially when 
a unregister/register event leads to other services being registered/unregisterd. One way to deal with 
this sync problem is to remove it by adding a single thread which handle al interaction between bundles 
and the service registry.

# Create dfi descriptor generator
Create a descriptor generator which can parse type and service headers. 
This can be relatively easy  achieved by (e.g.) using the libclang library. 
Also extend the CMake add_bundle command and create a CMake bundle_add_descriptors command to be able
to add the descriptors to the bundle

# Use dfi descriptor in service registrations/references
When available dfi descriptors can be used in Celix to validate if service are compatible and
in case of the provided service having more functions than the consumer needs, create a (cast to) 
a compatible consumer version.

# Fix the performance use case and add C++ support
There was a locking example which also functioned as a performance test to measure the impact of
using services instead of direct function calls. 
During the removal of APR this example stopped working. Add it back, fix it and add a C++ use case
as example.
See [locking example tree](https://github.com/apache/celix/tree/216032cae956379d4a740f37ae5caee7e957bd98/examples/locking)

# Cleanup Celix project structure
Celix is growing and getting more sub project, this is fine but maybe the structure needs a cleanup to 
create a more clear root directory structure. 

# Add Support for running Celix as a single executable
For different reasons it could be interesting to support running bundles from a single executable.
Primarily security, but also startup performance.


To start this can be achieved by creating static version of the celix provided bundles. These static 
bundles should have unique symbols, e.g  shell_bundleActivator_create instead of a 
"normal" bundleActivator_create. The normal  activator functions should then 
only be used (using a define or separated source file) when building a shared library bundle.
A additional property (e.g. "cosgi.auto.static.start=shell, shell_tui") can be used to specify which 
symbol prefixes to use.

Note that if this is implemented it would also be a good moment to add a compile option / launcher option
to disable dynamic loading of libraries. This way shared library based bundles can be used during 
development, but for production a static executable with no capability to add bundles runtime 
could be used if preferable. 

# Extend the test environment

# Improve documentation

# Create one or more "real life" applications based on Celix to show the potential of Celix

# Investigate if in the pubsub admin the serialization and communication can be decoupled. 
Now an admin is responsible for serialization and communication.

# Add pub sub admins. 
The current implementation uses JSON over multicast UDP or over ZMQ.  
One or more could be added. i.e. serialization based on Apache-Avro, communication over TCP / Kafka / Shared Memory  
Add interfaces for other languages (Python / Rust / Go / ...)  