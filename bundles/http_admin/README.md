---
title: HTTP Admin
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

## HTTP Admin

The HTTP admin provides a service tracker and starts a HTTP web server. The civetweb web server is used as an embedded
web server. Websockets are supported by the HTTP admin.

Services can register and implement HTTP requests or websocket support for a specified URI.
The supported HTTP requests are: GET, HEAD, POST, PUT, DELETE, TRACE, OPTIONS and PATCH.
The websocket service can support different callback handlers: connect, ready, data and close.

Aliasing is also supported for both HTTP services and websocket services. Multiple aliases can be added by using the comma as seperator.
Adding aliasing is done by adding the following function to the target CMakeFile (fill in <Alias path> and <Path to destination>):

```CMake
celix_bundle_headers(<TARGET>
        "X-Web-Resource: /<Alias path>$<SEMICOLON>/<Path to destination>, /<Alias path 2>$<SEMICOLON>/<Path to destination>"
)
```

Bundle alias resources can be added by the with the following function in the bundle CMakefile:

```CMake
celix_bundle_add_dir(<TARGET> <Document root of bundle> DESTINATION ".")
```

### Celix supported config.properties
    CELIX_HTTP_ADMIN_LISTENING_PORTS                 default = 8080, can be multiple ports divided by a comma
    CELIX_HTTP_ADMIN_PORT_RANGE_MIN                  default = 8000
    CELIX_HTTP_ADMIN_PORT_RANGE_MAX                  default = 9000
    CELIX_HTTP_ADMIN_USE_WEBSOCKETS                  default = true
    CELIX_HTTP_ADMIN_WEBSOCKET_TIMEOUT_MS            default = 3600000
    CELIX_HTTP_ADMIN_NUM_THREADS                     default = 1

## CMake option
    BUILD_HTTP_ADMIN=ON
