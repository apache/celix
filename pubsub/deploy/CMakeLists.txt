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
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.


# UDP Multicast
add_deploy("pubsub_publisher_udp_mc" 
    GROUP "pubsub"
    BUNDLES
       shell
       shell_tui
       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
       org.apache.celix.pubsub_admin.PubSubAdminUdpMc
       org.apache.celix.pubsub_publisher.PoiPublisher
       org.apache.celix.pubsub_publisher.PoiPublisher2
       org.apache.celix.pubsub_serializer.PubSubSerializerJson
)

add_deploy("pubsub_subscriber_udp_mc" 
    GROUP "pubsub"
    BUNDLES
       shell
       shell_tui
       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
       org.apache.celix.pubsub_admin.PubSubAdminUdpMc
       org.apache.celix.pubsub_subscriber.PoiSubscriber
       org.apache.celix.pubsub_serializer.PubSubSerializerJson
)

add_deploy("pubsub_subscriber2_udp_mc" 
    GROUP "pubsub"
    BUNDLES
       shell
       shell_tui
       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
       org.apache.celix.pubsub_admin.PubSubAdminUdpMc
       org.apache.celix.pubsub_subscriber.PoiSubscriber
       org.apache.celix.pubsub_serializer.PubSubSerializerJson
)

if (BUILD_PUBSUB_PSA_ZMQ)

	# Dynamic ZMQ / UDP admin
	add_deploy("pubsub_publisher"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
	       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_admin.PubSubAdminUdpMc
	       org.apache.celix.pubsub_publisher.PoiPublisher
	       org.apache.celix.pubsub_publisher.PoiPublisher2
	       org.apache.celix.pubsub_serializer.PubSubSerializerJson
	    PROPERTIES
	       poi1.psa=zmq
	       poi2.psa=udp
	)

	add_deploy("pubsub_subscriber"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
	       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_admin.PubSubAdminUdpMc
	       org.apache.celix.pubsub_subscriber.PoiSubscriber
	       org.apache.celix.pubsub_serializer.PubSubSerializerJson
	    PROPERTIES
	       poi1.psa=zmq
	       poi2.psa=udp
	)

	# ZMQ
	add_deploy("pubsub_zmq"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
	       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_publisher.PoiPublisher
	       org.apache.celix.pubsub_subscriber.PoiSubscriber
	       org.apache.celix.pubsub_serializer.PubSubSerializerJson
	)

	add_deploy("pubsub_publisher_zmq"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
	       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_publisher.PoiPublisher
	       org.apache.celix.pubsub_publisher.PoiPublisher2
	       org.apache.celix.pubsub_serializer.PubSubSerializerJson
	   	PROPERTIES
		    pubsub.scope=my_small_scope
	)

	add_deploy("pubsub_subscriber_zmq"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
	       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_subscriber.PoiSubscriber
	       org.apache.celix.pubsub_serializer.PubSubSerializerJson
	)

	add_deploy("pubsub_subscriber2_zmq"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
	       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_subscriber.PoiSubscriber
	       org.apache.celix.pubsub_serializer.PubSubSerializerJson
	)

	# ZMQ Multipart
	add_deploy("pubsub_mp_subscriber_zmq"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
	       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_subscriber.MpSubscriber
	       org.apache.celix.pubsub_serializer.PubSubSerializerJson
	)

	add_deploy("pubsub_mp_publisher_zmq"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
	       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_publisher.MpPublisher
	       org.apache.celix.pubsub_serializer.PubSubSerializerJson
	)

endif()
