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

# Shell

The Celix Shell provides a service interface which can be used to interact with the Celix framework. Note that it does not offer a user interface. This modular approach enables having multiple frontends, e.g. textual or graphical.

While the shell can be extended with additional commands by other bundles, it already offers some built in commands:

    lb            list bundles
    install       install additional bundle
    uninstall     uninstall bundles
    update        update bundles

    start         start bundle
    stop          stop bundle

    help          displays available commands
    inspect       inspect service and components

    log           print log

Further information about a command can be retrieved by using `help` combined with the command.

## CMake options
    BUILD_SHELL=ON

## Shell Config Options

- SHELL_USE_ANSI_COLORS - Whether shell command are allowed to use
ANSI colors when printing info. default is true.

## Using info

If the Celix Shell is installed, 'find_package(Celix)' will set:
 - The `Celix::shell_api` interface (i.e. header only) library target
 - The `Celix::shell` bundle target
