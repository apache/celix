---
title: PSA TCP 
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

# PUBSUB-Admin TCP 

---

## Description

The scope of this description is the TCP PubSub admin. 

The TCP pubsub admin is used to transfer user data transparent via TCP.

### IP Addresses 

To use TCP, 1 IP address is needed:
This is the IP address that is bound to an (ethernet) interface (from the publisher). 
This IP address is defined with  the `PSA_IP` property.
For example: `PSA_IP=192.168.1.0/24`.
Note the example uses CIDR notation, the CIDR notation specifies the IP range that is used
by the pubsub admin for searching the network interfaces.

Note the port will be automatically assigned, when none is specified.
A fixed port can be assigned for example with: `PSA_IP=192.168.1.0/24:34000`.


### Discovery

When a publisher wants to publish a topic a TopicSender and publisher endpoint is created by the Pubsub Topology Manager.
When a subscriber wants to subscribe to topic a TopicReceiver and subscriber endpoint is created by the Pubsub Topology Manager.
The endpoints are published by the PubSubDiscovery within its topic in ETCD (i.e. udp://192.168.1.20:40123).
A subscriber, interested in the topic, is informed by the TopologyManager that there is a new endpoint. 
The TopicReceiver at the subscriber side creates a listening socket based on this endpoint.
Now a data-connection is created and data send by the publisher will be received by the subscriber.  

### Static endpoint

TCP pubsub admin also be used as a static endpoint that can be used to communicate with external
TCP publish/subsribers that don't use Discovery.
---
With the `tcp.static.bind.url` property, the static UDP bind can be configured. 
For TCP the publisher topic properties should configure the `udp.static.bind.url` property to configure TCP bind interface
that accepts incomming connections.

---
With the `udp.static.connect.urls` property, the static TCP connections can be configured. (Note The urls are space separate)
For TCP the subscriber topic properties should configure the `udp.static.connect.urls` property 
to configure TCP connections that subsribe to the data.

---
Note a special/dedicated protocol service implemenation can be used to communcate with external publisher/subscribers 

---

## Passive Endpoints

Each TCP pubsub publisher/subscriber can be comfigured as a passive endpoint. 
A passive endpoint reuses an existing socket of a publisher/subscriber using properties.
This can be used to support full duplex socket connections.
This can be used for both discovery and static end points.

The topic `tcp.passive.key` property defines the unique key to select the socket.
The `tcp.passive.key` property needs to be defined both in the topic properties of the topic connections that will be shared.
and the topic properties of the 'passive' topic that will re-use the connection.
The `tcp.passive.configured` topic property will indicate that is topic is passive and will re-use the connections
indicated with the `tcp.passive.key` property

For example: `tcp.passive.key=udp://localhost:9500` and `tcp.passive.configured=true`

## IOVEC 

The TCP pubsub uses the socket function calls sendnsg / recvmsg with iovec for sending/receiving messages.
When the serialisation service supports iovec serialisation.
iovec serialisation can be used for high throughput and low latency serialisation with avoiding of copying 
of data. Because of iovec limitations the protocol service should support message segmentation.

## Properties

<table border="1">
    <tr><th>Property</th><th>Description</th></tr>
    <tr><td>PSA_IP</td><td>IP address that is used by the bundle</td></tr>
    <tr><td>tcp.static.bind.url</td><td>The static interface url of the bind interface</td></tr>
    <tr><td>tcp.static.connect.urls</td><td>a space seperated list with static connections urls</td></tr>
    <tr><td>tcp.passive.key</td><td>key of the shared connection</td></tr>
    <tr><td>tcp.passive.configured</td><td>Indicates if the connection is passive and reuses the connection</td></tr>
    <tr><td>thread.realtime.prio</td><td>Configures the thread priority of the receive thread</td></tr>
    <tr><td>thread.realtime.sched</td><td>Configures the thread scheduling type of the receive thread</td></tr>

</table>

---



