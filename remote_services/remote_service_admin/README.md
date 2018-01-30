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

## Remote Service Admin

The Remote Service Admin (RSA) provides the mechanisms to import and export services when instructed to do so by the Topology Manager. 

To delegate method calls to the actual service implementation, the RSA_SHM and the RSA_HTTP are using "endpoint/proxy" bundles, which has all the knowledge about the marshalling and unmarshalling of data for the service. The RSA_DFI implementation combines a [foreign function interface](https://en.wikipedia.org/wiki/Foreign_function_interface) technique together with manualy created descriptors.  

Note that this folder contains code commonly used by the RSA implementations and therefore does not include any CMAKE configuration.

###### Properties
    ENDPOINTS				 defines the relative directory where endpoints and proxys can be found (default: endpoints)
    CELIX_FRAMEWORK_EXTENDER_PATH  Used in RSA_DFI only. Can be used to define a path to use as an extender path point for the framework bundle. For normal bundles the bundle cache is used. 
