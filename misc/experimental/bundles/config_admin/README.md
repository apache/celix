---
title: Configuration Admin
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

# Configuration Admin

---

## Introduction
The configuration Admin service allows defining and deploying configuration data to bundles.
When compared to config.properties it adds the option to update configuration data by providing a persistent storage. It also allows changing configuration data at run-time.

---

## Design

The config_admin bundle implements the configuration_admin service, the interface to configuration objects and the interface of a managed service. At the moment, the implementation uses a config_admin_factory to generate config_admin services for each bundle that wants to use this service. This is an inheritance of the original design and not needed.
The configuration data is stored persistently in a subdirectory store of the current bundle directory. 
The filenames have the name of the PID and have an extension pid, e.g. base.device1.pid
The files contains a list of key/value pairs. At least the following keys need to be present:
service.bundleLocation
service.pid

---

## TODO

1. Test the configuration of a service_factory
2. Think about the option to allow remote update of the managed_services
3. Support configuration of multiple managed services with the same PID. At the moment, only one service is bound to a configuration object.
   To support this the getConfiguration2 function needs to be called with a location NULL according to the spec.

---

## Usage

1. Bundle that needs configuration data
   This bundle has to register next to its normal service a managed service that has an update method. This managed service needs to be registered with a properties object that contains the key/value pair service.pid=<PID NAME>.
 Use config_admin_tst/example_test as an example (it is better than example_test2)
2. Bundle/application that wants to update the configuration data of the system
   This bundle needs to retrieve the running config_admin service. With this service it can retrieve all configuration objects for all known Persistent Identifiers (PIDs). For each PID, get all properties that need to be updated. See config_admin_test for an example.
