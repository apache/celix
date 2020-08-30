---
title: Discovery ETCD
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

## Discovery ETCD

The Celix Discovery ETCD bundles realizes OSGi services discovery based on [etcd](https://github.com/coreos/etcd).

###### Properties
    DISCOVERY_ETCD_ROOT_PATH            Used path to announce and find discovery entpoints (default: discovery)
    DISCOVERY_ETCD_SERVER_IP            ip address of the etcd server (default: 127.0.0.1)
    DISCOVERY_ETCD_SERVER_PORT          port of the etcd server  (default: 2379)
    DISCOVERY_ETCD_TTL                  time-to-live for etcd entries in seconds (default: 30)
    
    DISCOVERY_CFG_SERVER_IP             The host to use/announce for this framewokr discovery endpoint. Default "127.0.0.1"
    DISCOVERY_CFG_SERVER_PORT           The port to use/announce for this framework endpoint endpoint. Default 9999
    DISCOVERY_CFG_SERVER_PATH           The path to use for this framework discovery endpoint.Default "/org.apache.celix.discovery.etcd"

###### CMake option
    BUILD_RSA_DISCOVERY_ETCD=ON
