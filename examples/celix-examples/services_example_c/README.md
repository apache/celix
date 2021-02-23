---
title: Services example C
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

# Intro

These examples shows howto create and interact with services by example.

In both examples there is a provider and consumer bundle.
The provider bundle creates services and registers them to the Celix framework so that these services are available for use.
And the consumer bundle request the services from the Celix framework and uses them.

The examples use the `celix_bundleContext_registerService` to provide
services and uses a combination of `celix_bundleContext_useService`,
`celix_bundleContext_useServices` and `celix_bundleContext_trackServices`
to consume services.

The bundle context is the proxy to the Celix framework for a bundle
and can be used to:

- Register services
- Register service factories
- Use services
- Track services
- Track bundles
- Track service trackers

See the `bundle_context.h` for documentation about these - and other -functions

# Simple Service Provider & Consumer Example

The simple provider/consumer example can be executed by launching the
`services_example_c` executable target
(build in the directory `${CMAKE_BUILD_DIR}/deploy/c_examples/services_example_c`)

In this example the provider bundle only registers 1 calc service. And
the consumer bundle tries to use this during startup and registered
a service tracker for the calc service.

```ditaa
+----------------------+                  +------------------------+
|                      |                  |                        |
|                      |                  |                        |
|   provider_example   |   example_calc   |    consumer_example    |
|                      +--------O)--------+                        |
|       <bundle>       |                  |       <bundle>         |
|                      |                  |                        |
|                      |                  |                        |
+----------------------+                  +------------------------+
```

Try stopping/starting the provider/consumer bundles (respectively bundle id 3 & 4) using the interactive shell
to see how this works at runtime. 
I.e. type `stop 3`, `stop 4`, `start 3`, `start 4` in different combinations.


# Dynamic Service Provider & Consumer Example

The dynamic provider/consumer example can be executed by launching the
`dynamic_services_example_c` executable target
(build in `${CMAKE_BUILD_DIR}/deploy/c_examples/dynamic_services_example_c`)

The dynamic service provide / consumer example show how the framework copes
with the dynamic behaviour of services.

This this example the provided dynamically register more and less calc services in a thread.
The consumer bundle uses these services in a every 5 seconds and print some info.

This example should give an idea how services can be safely used and tracked in a dynamic environment.

