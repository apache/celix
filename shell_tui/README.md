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

# Shell TUI

The Celix Shell TUI implements a textual user interface for the Celix Shell.

## CMake option
    BUILD_SHELL_TUI=ON

## Config options

- SHELL_USE_ANSI_CONTROL_SEQUENCES - Wether to use ANSI control
sequences to support backspace, left, up, etc key commands in the
shell tui. Default is true if a TERM environment is set else false.

## Using info

If the Celix Shell TUI is installed, 'find_package(Celix)' will set:
 - The `Celix::shell_tui` bundle target if the shell_tui is installed
