# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#  KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
 
The example activates the bundles `chapter01-greeting-example` and  `chapter01-greeting-example-client`.
The activator of the client bundle (client.c) tries to find the greeting service, but may fail at start-up,
because it is activated before the greeting service is installed.

When the text 'Greetings' does not appear, stop and start the `chapter01-greeting-example-client` bundle.
Now the `chapter01-greeting-example` is already installed and the text `Greetings` does appear after the 
restart of the client bundle.
