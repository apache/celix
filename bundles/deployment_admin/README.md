---
title: Deployment Admin
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

## Deployment Admin

The Celix Deployment Admin implements the OSGi Deployment Admin specification, which provides functionality to manage deployment packages. Deployment package are bundles and other artifacts that can be installed, updated and uninstalled as single unit.

It can be used for example with Apache Ace, which allows you to centrally manage and distribute software components, configuration data and other artifacts.

###### Properties
                  tags used by the deployment admin

## CMake option
    BUILD_DEPLOYMENT_ADMIN=ON

## Deployment Admin Config Options

- deployment_admin_identification     id used by the deployment admin to identify itself
- deployment_admin_url                url of the deployment server
- deployment_cache_dir                possible cache dir for the deployment admin update
- deployment_tags

## Using info

If the Celix Deployment Admin is installed, 'find_package(Celix)' will set:
 - The `Celix::deployment_admin_api` interface (i.e. headers only) library target
 - The `Celix::deployment_admin` bundle target
