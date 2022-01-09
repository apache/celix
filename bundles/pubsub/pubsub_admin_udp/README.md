---
title: PSA UDP 
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

# PUBSUB-Admin UDP 

---

## Description

The scope of this description is the UDP PubSub admin. 

The UDP pubsub admin is used to transfer user data transparent via UDP unicast, UDP broadcast and UDP multicast.
UDP support packets with a maximum of 64kB . To overcome this limit the admin uses a protocol service on top of UDP 
which fragments the data to be send and these fragments are reassembled at the reception side.

### IP Addresses Unicast

To use UDP-unicast, 1 IP address is needed:
This is the IP address that is bound to an (ethernet) interface (from the subscriber). 
This IP address is defined with  the `PSA_IP` property.
For example: `PSA_IP=192.168.1.0/24`.
Note the example uses CIDR notation, the CIDR notation specifies the IP range that is used
by the pubsub admin for searching the network interfaces.

Note the port will be automatically assigned, when none is specified.
A fixed port can be assigned for example with: `PSA_IP=192.168.1.0/24:34000`.

### IP Address Mulicast

To use UDP-multicast 2 IP addresses are needed:

1. The multicast address (in the range 224.X.X.X - 239.X.X.X)
2. IP address which is bound to an (ethernet) interface (from the publisher)

These IP address are defined with the `PSA_IP` property, 
with the definition multicast ip address at (ethernet) interface ip address
For example: `PSA_IP=224.100.0.0/24@192.168.1.0/24`.
Note the example uses CIDR notation, the CIDR notation specifies the IP range that is used
by the pubsub admin for searching the network interfaces. 
The multicast address will use the last digits of the network interface IP address 
for an unique multicast address.   

Note the port will be automatically assigned, when none is specified.
A fixed port can be assigned for example with: `PSA_IP=224.100.0.0/24:34000@192.168.1.0/24`.

### IP Address Broadcast

To use UDP-broad 2 IP addresses are needed:

1. The broadcast address (X.X.X.255)
2. IP address which is bound to an (ethernet) interface (from the publisher)

These IP address are defined with the `PSA_IP` property,
with the definition broadcast ip address at (etnernet) interface ip address
For example: `PSA_IP=192.168.1.255@192.168.1.0/24`.
Note the example uses CIDR notation, the CIDR notation specifies the IP range that is used
by the pubsub admin for searching the network interfaces. 

Note the port will be automatically assigned, when none is specified.
A fixed port can be assigned for example with: `PSA_IP=192.168.1.255:34000@192.168.1.0/24:34000`.


### Discovery

When a publisher wants to publish a topic a TopicSender and publisher endpoint is created by the PubSub Topology Manager.
When a subscriber wants to subscribe to topic a TopicReceiver and subscriber endpoint is created by the Pubsub Topology Manager.
The endpoints are published by the PubSubDiscovery within its topic in ETCD (i.e. udp://192.168.1.20:40123).
A subscriber, interested in the topic, is informed by the TopologyManager that there is a new endpoint. 
The TopicReceiver at the subscriber side creates a listening socket based on this endpoint.
Now a data-connection is created and data send by the publisher will be received by the subscriber.  


### Static endpoint

UDP pubsub admin also be used as a static endpoint that can be used to communicate with external
UDP publish/subsribers that don't use Discovery.
---
With the `udp.static.bind.url` property, the static UDP bind can be configured.
1. For UDP unicast the subscriber topic properties should configure the `udp.static.bind.url` property to configure UDP destination.
2. For UDP multicast the publisher topic properties should configure the `udp.static.bind.url` property to configure UDP multicast bind interface.
3. For UDP broadcast the publisher topic properties should configure the `udp.static.bind.url` property to configure UDP broadcast bind interface.
---
With the `udp.static.connect.urls` property, the static UDP connections can be configured. (Note The urls are space separate)
1. For UDP unicast the publisher topic properties should configure the `udp.static.connect.urls` property to configure UDP connections were to send the data.
2. For UDP multicast the subscriber topic properties should configure the `udp.static.connect.urls` property to configure UDP multicast connection.
3. For UDP broadcast the subscriber topic properties should configure the `udp.static.connect.urls` property to configure UDP broadcast connection.
---
Note a special/dedicated protocol service implemenation can be used to communcate with external publisher/subscribers 

---

## Passive Endpoints

Each UDP pubsub publisher/subscriber can be comfigured as a passive endpoint. 
A passive endpoint reuses an existing socket of a publisher/subscriber using properties.
This can be used to support full duplex socket connections.
This can be used for both discovery and static end points.

The topic `udp.passive.key` property defines the unique key to select the socket.
The `udp.passive.key` property needs to be defined both in the topic properties of the topic connections that will be shared.
and the topic properties of the 'passive' topic that will re-use the connection.
The `udp.passive.configured` topic property will indicate that is topic is passive and will re-use the connections
indicated with the `udp.passive.key` property

For example: `udp.passive.key=udp://localhost:9500` and `udp.passive.configured=true`

## IOVEC 

The UDP pubsub uses the socket function calls sendnsg / recvmsg with iovec for sending/receiving messages.
When the serialisation service supports iovec serialisation.
iovec serialisation can be used for high throughput and low latency serialisation with avoiding of copying 
of data. Because of iovec limitations the protocol service should support message segmentation.

## Properties

<table border="1">
    <tr><th>Property</th><th>Description</th></tr>
    <tr><td>PSA_IP</td><td>Unicast/Multicast/Broadcast IP address that is used by the bundle</td></tr>
    <tr><td>udp.static.bind.url</td><td>The static interface url of the bind interface</td></tr>
    <tr><td>udp.static.connect.urls</td><td>a space seperated list with static connections urls</td></tr>
    <tr><td>udp.passive.key</td><td>key of the shared connection</td></tr>
    <tr><td>udp.passive.configured</td><td>Indicates if the connection is passive and reuses the connection</td></tr>
    <tr><td>thread.realtime.prio</td><td>Configures the thread priority of the receive thread</td></tr>
    <tr><td>thread.realtime.sched</td><td>Configures the thread scheduling type of the receive thread</td></tr>

</table>

---



