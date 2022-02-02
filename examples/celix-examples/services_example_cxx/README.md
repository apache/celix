---
title: Services Example C++
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

These examples show howto create and interact with services by example.

In both examples there is a provider and consumer bundle.
The provider bundle creates services and registers them to the Celix framework so that these services are available for use.
And the consumer bundle request the services from the Celix framework and uses them.

The examples use the `celix::BundleContext::registerService` to provide
services and uses a `celix::BundleContext::trackServices` to consume services.

The bundle context is the proxy to the Celix framework for a bundle
and can be used to:

- Register services
- Use services
- Track services
- Track bundles
- Track service trackers

See the `celix/BundleContext.h` for documentation about these - and other -functions

# Simple Service Provider & Consumer Example

The simple provider/consumer example can be executed by launching the
`SimpleServicesExample` executable target
(build in `${CMAKE_BUILD_DIR}/deploy/cxx_examples/SimpleServicesExample`)

In this example the provider bundle only register one calc service. And
the consumer bundle uses this service with use of a service tracker.

```ditaa
+----------------------+                  +------------------------+
|                      |                  |                        |
|                      |                  |                        |
|   SimpleProvider     | examples::ICalc  |    SimpleConsumer      |
|                      +--------O)--------+                        |
|       <bundle>       |                  |       <bundle>         |
|                      |                  |                        |
|                      |                  |                        |
+----------------------+                  +------------------------+
```

The effect of this can be examed by stopping and starting the provider/consumer bundles (respectively bundle id 3 & 4) using the interactive shell
to see how this works at runtime.
I.e. type `stop 3`, `stop 4`, `start 3`, `start 4` in different combinations.


# Dynamic Service Provider & Consumer Example

The dynamic provider/consumer example can be executed by launching the
`DynamicServicesExample` executable target
(build in `${CMAKE_BUILD_DIR}/deploy/cxx_examples/DynamicServicesExample`)

The dynamic service provide / consumer example show howto handle the dynamic behavior
of services. 

In this example the provider dynamically register more and less ICalc services in a thread.
The consumer create and destroy consumer for the ICalc services.

This example should give an idea how services can be safely used and tracked in a dynamic environment.