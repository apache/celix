---
title: Remote Service Admin DFI
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

## Remote Service Admin DFI

The Celix Remote Service Admin DFI bundle realizes OSGi remote service using HTTP and JSON.
The serialization is done using libdfi to json.
Libffi is configured using descriptor files in the bundles.

###### Properties/Configuration
    RSA_PORT                    The RSA HTTP port to use (default 8888)
    RSA_IP                      The RSA ip address to use for discovery (default 127.0.0.1)
    RSA_INTERFACE               If specified, the ip adress of interface (i.g. eth0) will be used.
    
    RSA_LOG_CALLS              If set to true, the RSA will Log calls info (including serialized data) to the file in RSA_LOG_CALLS_FILE. Default is false.
    RSA_LOG_CALLS_FILE         If RSA_LOG_CALLS is enabled to file to log to (starting rsa will truncate file). Default is stdout.   

    RSA_DFI_USE_CURL_SHARE_HANDLE   If set to true the RSA will use curl's share handle. 
                                    The curl share handle has a significant performance boost by sharing DNS, COOKIE en CONNECTIONS over multiple calls, 
                                    but can also introduce some issues (based on experience).
                                    Default is false
    CELIX_RSA_BIND_ON_ALL_INTERFACES    If set to true the RSA will bind to all interfaces. Default is true.

    CELIX_RSA_DFI_DYNAMIC_IP_SUPPORT    If set to true the RSA will support dynamic IP address fill-in for service exports. Default is false.

###### CMake option
    RSA_REMOTE_SERVICE_ADMIN_DFI=ON
