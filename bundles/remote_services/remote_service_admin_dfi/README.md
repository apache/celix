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

###### Properties
    RSA_PORT                    The RSA HTTP port to use (default 8888)
    RSA_IP                      The RSA ip address to use for discovery (default 127.0.0.1)
    RSA_INTERFACE               If specified, the ip adress of interface (i.g. eth0) will be used.
    
    RSA_LOG_CALLS              If set to true, the RSA will Log calls info (including serialized data) to the file in RSA_DUMP_CALLS_FILE. Default is false.
    RSA_LOG_CALLS_FILE         If RSA_DUMP_CALLS is enabled to file to dump to (starting rsa will truncate file). Default is stdout.          

###### CMake option
    RSA_REMOTE_SERVICE_ADMIN_DFI=ON
