---
title: PSA UDP Multicast
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

# PUBSUB-Admin UDP Multicast

---

## Description

This description is particular for the UDP-Multicast PUB-SUB. 

The UDP multicast pubsub admin is used to transfer user data transparent via UDP multicast. UDP packets can contain approximately  
64kB . To overcome this limit the admin has a protocol on top of UDP which fragments the data to be send and these  
fragments are reassembled at the reception side.

### IP Addresses

To use UDP-multicast 2 IP addresses are needed:

1. IP address which is bound to an (ethernet) interface
2. The multicast address (in the range 224.X.X.X - 239.X.X.X)

When the PubSubAdmin starts it determines the bound IP address. This is done in the order:

1. The first IP number bound to the interface which is set by the "PSA_INTERFACE" property
2. The interfaces are iterated and the first IP number found is used. (typically this is 127.0.0.1 (localhost)

The multicast IP address is determined in the order:

1. If the `PSA_IP` property is defined, this IP will be used as multicast.
2. If the `PSA_MC_PREFIX` property, is defined, this property is used as the first 2 numbers of the multicast address extended with the last 2 numbers of the bound IP.
3. If the `PSA_MC_PREFIX` property is not defined `224.100` is used.

### Discovery

When a publisher request for a topic a TopicSender is created by a ServiceFactory. This TopicSender uses the multicast address as described above with a random chosen portnumber. The combination of the multicast-IP address with the portnumber and protocol(udp) is the endpoint.  
This endpoint is published by the PubSubDiscovery within its topic in ETCD (i.e. udp://224.100.10.20:40123).
 
A subscriber, interested in the topic, is informed by the the TopologyManager that there is a new endpoint. The TopicReceiver at the subscriber side creates a listening socket based on this endpoint.

Now a data-connection is created and data send by the publisher will be received by the subscriber.  

---

## Properties

<table border="1">
    <tr><th>Property</th><th>Description</th></tr>
    <tr><td>PSA_INTERFACE</td><td>Interface which has to be used for multicast communication</td></tr>
    <tr><td>PSA_IP</td><td>Multicast IP address used by the bundle</td></tr>
    <tr><td>PSA_MC_PREFIX</td><td>First 2 digits of the MC IP address </td></tr>
</table>

---

## Shortcomings

1. Per topic a random portnr is used for creating an endpoint. It is theoretical possible that for 2 topic the same endpoint is created.
2. For every message a 32 bit random message ID is generated to discriminate segments of different messages which could be sent at the same time. It is theoretically possible that there are 2 equal message ID's at the same time. But since the message ID is valid only during the transmission of a message (maximum some milliseconds with large messages) this is not very plausible.
3. When sending large messages, these messages are segmented and sent after each other. This could cause UDP-buffer overflows in the kernel. A solution could be to add a delay between sending of the segments but this will introduce extra latency.
4. A Hash is created, using the message definition, to identify the message type. When 2 messages generate the same hash something will terribly go wrong. A check should be added to prevent this (or another way to identify the message type). This problem is also valid for the other admins.




