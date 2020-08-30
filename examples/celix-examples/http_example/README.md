---
title: HTTP Example
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

# HTTP Example

This examples shows how the Celix HTTP Admin can be used. 

This examples contains a `http_example` example bundle and uses the `Celix::http_admin` `Celix::shell` and `Celix::shell_wui` 
bundles to create a `http_example_cnt` Celix container.

The `http_example_cnt` Celix container shows how you can create a use existing http/websocket services.
After running the container browse to localhost:8080. 
Under the /shell uri you find the `Celix::shell_wui` functionality and under /hello uri you find the `http_example` functionality
