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

# Roadmap

Note this roadmap is still a draft.

# Apache Celix 2.0.1 

Date: TBD (juli/aug 2017?)

## Improve PubSub (CELIX-407)

1. Finalize introduction serializer services  
1. Ensure code coverage of ~ 70% 

## Finalize Runtime Creation (CELIX-408)

1. Ensure that the runtime command are used for testing some distributed test (e.g. pubsub)

## Add Support for running Celix as a single executable (TODO issue)
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

# Apache Celix 2.1.0

Date: TBD (Jan 2018)

Note a short comming is that there is still no good support for export and import libraries between
bundles. For Linux dlmopen is a solution, but for a more broader UNIX support a more "creative" 
solution is needed (e.g. Just in time replacing the SONAME and NEEDED values in the respective shared libraries)

## Extend Dependency Manager C/C++ (CELIX-409)

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
1. Getting framework properties (e.g. bundleContext_getProperty)
1. Support for getting basic info from bundle (version, symbolic name, manifest)

## Add framework services (TODO issue)
Apache Celix is a framework for service oriented programming, this should also include for interacting with the 
framework itself. Create framework services for:

1. bundle management: installing, Starting, Stopping, deinstalling, listing, etc bundles.
1. service registry info: nr of service registered, nr of service listeners, etc.
1. framework listenerL start, stop, error events (whiteboard)
1. bundle listener: installed, start, stop, deinstalled events (whiteboard)

## Support dependency manager from framework (TODO issue)
Because the dependency manager is actual the preferred way to write bundles, this should be support
like "normal" activators" directly from the framework without using a static library.

## Refactor Celix provided bundles to use dependency manager and framework services (TODO issue)
Refactor Celix bundles (shell, shell_tuu, RSA, etc) to use the dependency manager instead of a "vanilla" 
bundle activator. The dependency manager should be the preferred way to handle services. 
Also instead of directly using the API from bundle_context, bundle, module, etc the framework services
should be used.

## Create dfi descriptor generator (TODO issue)
Create a descriptor generator which can parse type and service headers. 
This can be relatively easy  achieved by (e.g.) using the libclang library. 
Also extend the CMake add_bundle command and create a CMake bundle_add_descriptors command to be able
to add the descriptors to the bundle

## Use dfi descriptor in service registrations/references (TODO issue)
When available dfi descriptors can be used in Celix to validate if service are compatible and
in case of the provided service having more functions than the consumer needs, create a (cast to) 
a compatible consumer version.

# Apache Celix 3.0.0

Date: TBD (aug 2018)

## Remove support for "vanilla" bundle activators (TODO issue)
When all Celix provided bundles are using the dependency manager and framework services support for
the "vanilla"  bundle activator can be dropped. Also a lot internals can be refactored because the public
API should have shrunk quite a bit; This should lead to smaller code base -> less complex, easier to maintain
and a smaller footprint.

  
## Refactor service registry to a single threaded design (TODO issue)
Celix currently has some nested lock. It would be preferable to remove these, but this is difficult 
because they are used to sync events which react on registering/unregistering services. Specially when 
a unregister/register event leads to other services being registered/unregisterd. One way to deal with 
this sync problem is to remove it by adding a single thread which handle al interaction between bundles 
and the service registry.

