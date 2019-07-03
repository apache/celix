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

# TODO integrate cxx dep man into framework

- Move documentation
- Move dummy targets
- Deprecate the Celix::dependency_manager_cxx_static target. (how?)
- Eventually remove the deprecated cxx dm target
- The bundle activator is now still a small .cc file, still resulting in
a static library which has to be linked as whole. Make this a src dependency? or some how a
header impl ?