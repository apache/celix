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

# Improvement Ideas
 
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


# Fix the performance use case and add C++ support
There was a locking example which also functioned as a performance test to measure the impact of
using services instead of direct function calls. 
During the removal of APR this example stopped working. Add it back, fix it and add a C++ use case
as example.
See [locking example tree](https://github.com/apache/celix/tree/216032cae956379d4a740f37ae5caee7e957bd98/examples/locking)

# Cleanup Celix project structure
Celix is growing and getting more sub project, this is fine but maybe the structure needs a cleanup to 
create a more clear root directory structure. 

# Extend the test environment

# Improve documentation

# Create one or more "real life" applications based on Celix to show the potential of Celix

# Add pub sub admins. 
The current implementation uses JSON over multicast UDP or over ZMQ.  
One or more could be added. i.e. serialization based on Apache-Avro, communication over TCP / Kafka / Shared Memory  
Add interfaces for other languages (Python / Rust / Go / ...)  
