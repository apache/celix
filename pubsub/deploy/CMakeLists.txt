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

find_program(ETCD_CMD NAMES etcd)
find_program(XTERM_CMD NAMES xterm)

# UDP Multicast
add_deploy("pubsub_publisher_udp_mc" 
    GROUP "pubsub"
    BUNDLES
       shell
       shell_tui
		org.apache.celix.pubsub_serializer.PubSubSerializerJson
		org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
       org.apache.celix.pubsub_admin.PubSubAdminUdpMc
       org.apache.celix.pubsub_publisher.PoiPublisher
       org.apache.celix.pubsub_publisher.PoiPublisher2
)

add_deploy("pubsub_subscriber_udp_mc" 
    GROUP "pubsub"
    BUNDLES
       shell
       shell_tui
		org.apache.celix.pubsub_serializer.PubSubSerializerJson
		org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
       org.apache.celix.pubsub_admin.PubSubAdminUdpMc
       org.apache.celix.pubsub_subscriber.PoiSubscriber
)

add_deploy("pubsub_subscriber2_udp_mc" 
    GROUP "pubsub"
    BUNDLES
       shell
       shell_tui
		org.apache.celix.pubsub_serializer.PubSubSerializerJson
		org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
       org.apache.celix.pubsub_admin.PubSubAdminUdpMc
       org.apache.celix.pubsub_subscriber.PoiSubscriber
)

if (ETCD_CMD AND XTERM_CMD)
	#Runtime starting a publish and subscriber for udp mc
	add_runtime(pubsub_rt_upd_mc
		NAME udp_mc
		GROUP pubsub
		DEPLOYMENTS
			pubsub_publisher_udp_mc
			pubsub_subscriber_udp_mc
			pubsub_subscriber2_udp_mc
		COMMANDS
			etcd
		USE_TERM
	)
else ()
	message(WARNING "Cannot create runtime for pubsub because etcd and/or xterm is not installed")
endif ()

if (BUILD_PUBSUB_PSA_ZMQ)

	# Dynamic ZMQ / UDP admin
	add_deploy("pubsub_publisher"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
			org.apache.celix.pubsub_serializer.PubSubSerializerJson
			org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_admin.PubSubAdminUdpMc
	       org.apache.celix.pubsub_publisher.PoiPublisher
	       org.apache.celix.pubsub_publisher.PoiPublisher2
	    PROPERTIES
	       poi1.psa=zmq
	       poi2.psa=udp
	)

	add_deploy("pubsub_subscriber"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
			org.apache.celix.pubsub_serializer.PubSubSerializerJson
			org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_admin.PubSubAdminUdpMc
	       org.apache.celix.pubsub_subscriber.PoiSubscriber
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
			org.apache.celix.pubsub_serializer.PubSubSerializerJson
			org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_publisher.PoiPublisher
	       org.apache.celix.pubsub_subscriber.PoiSubscriber
	)

	add_deploy("pubsub_publisher_zmq"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
			org.apache.celix.pubsub_serializer.PubSubSerializerJson
			org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_publisher.PoiPublisher
	       org.apache.celix.pubsub_publisher.PoiPublisher2
	   	PROPERTIES
		    pubsub.scope=my_small_scope
	)

	add_deploy("pubsub_subscriber_zmq"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
			org.apache.celix.pubsub_serializer.PubSubSerializerJson
	       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_subscriber.PoiSubscriber
	)

	add_deploy("pubsub_subscriber2_zmq"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
			org.apache.celix.pubsub_serializer.PubSubSerializerJson
	       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_subscriber.PoiSubscriber

	)

	# ZMQ Multipart
	add_deploy("pubsub_mp_subscriber_zmq"
	    GROUP "pubsub"
	    BUNDLES
	       shell
	       shell_tui
			org.apache.celix.pubsub_serializer.PubSubSerializerJson
	       org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	       org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	       org.apache.celix.pubsub_admin.PubSubAdminZmq
	       org.apache.celix.pubsub_subscriber.MpSubscriber
	)

	add_deploy("pubsub_mp_publisher_zmq"
	    GROUP "pubsub"
	    BUNDLES
	   		shell
	   		shell_tui
	   		org.apache.celix.pubsub_serializer.PubSubSerializerJson
			org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
	   		org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
	   		org.apache.celix.pubsub_admin.PubSubAdminZmq
	   		org.apache.celix.pubsub_publisher.MpPublisher
	)

	if (ETCD_CMD AND XTERM_CMD)
		#Runtime starting two bundles using both zmq and upd mc pubsub
		add_runtime(pubsub_rt_zmq_udpmc_combi
			NAME combi
			GROUP pubsub
			DEPLOYMENTS
				pubsub_publisher_zmq
				pubsub_subscriber_zmq
				pubsub_subscriber_zmq
			COMMANDS
				etcd
			USE_TERM
		)

		#Runtime starting a publish and 2 subscribers for zmq
		add_runtime(pubsub_rt_zmq
			NAME zmq
			GROUP pubsub
			DEPLOYMENTS
				pubsub_publisher
				pubsub_subscriber_zmq
				pubsub_subscriber2_zmq
			COMMANDS
				etcd
			USE_TERM
		)

		#Runtime starting a multipart (multiple message in one send) publish and subscriber for zmq
		add_runtime(pubsub_rt_multipart_zmq
			NAME zmq_multipart
			GROUP pubsub
			DEPLOYMENTS
				pubsub_mp_subscriber_zmq
				pubsub_mp_publisher_zmq
			COMMANDS
				etcd
			USE_TERM
		)
	endif ()

endif()
