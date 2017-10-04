# Publisher / subscriber implementation

This subdirectory contains an implementation for a publish-subscribe remote services system, that use dfi library for message serialization.
For low-level communication, UDP and ZMQ is used.

# Description

This publisher / subscriber implementation is based on the concepts of the remote service admin (i.e. rsa / topology / discovery pattern).

Publishers are senders of data, subscribers can receive data. Publishers can publish/send data to certain channels (called 'topics' further on), subscribers can subscribe to these topics. For every topic a publisher service is created by the pubsub admin. This publisher is announced through etcd. So etcd is used for discovery of the publishers. Subscribers are also registered as a service by the pubsub admin and will watch etcd for changes and when a new publisher is announced, the subscriber will check if the topic matches its interests. If the subscriber is interested in/subscribed to a certain topic, a connection between publisher and subscriber will be instantiated by the pubsub admin.

The dfi library is used for message serialization. The publisher / subscriber implementation will arrange that every message which will be send gets an unique id. 

For communication between publishers and subscribers UDP and ZeroMQ can be used. When using ZeroMQ it's also possible to setup a secure connection to encrypt the traffic being send between publishers and subscribers. This connection can be secured with ZeroMQ by using a curve25519 key pair per topic.

The publisher/subscriber implementation supports sending of a single message and sending of multipart messages.

## Getting started

The publisher/subscriber implementation contains 2 different PubSubAdmins for managing connections:
  * PubsubAdminUDP: This pubsub admin is using linux sockets to setup a connection. 
  * PubsubAdminZMQ (LGPL License): This pubsub admin is using ZeroMQ and is disabled as default. This is a because the pubsub admin is using ZeroMQ which is licensed as LGPL ([View ZeroMQ License](https://github.com/zeromq/libzmq#license)).
  
  The ZeroMQ pubsub admin can be enabled by specifying the build flag `BUILD_PUBSUB_PSA_ZMQ=ON`. To get the ZeroMQ pubsub admin running, [ZeroMQ](https://github.com/zeromq/libzmq) and [CZMQ](https://github.com/zeromq/czmq) need to be installed. Also, to make use of encrypted traffic, [OpenSSL](https://github.com/openssl/openssl) is required.

## Running instructions

### Running PSA UDP-Multicast

1. Open a terminal
1. Run `etcd`
1. Open second terminal on project build location
1. Run `cd deploy/pubsub/pubsub_publisher_udp_mc`
1. Run `sh run.sh`
1. Open third terminal on project build location
1. Run `cd deploy/pubsub/pubsub_subscriber_udp_mc`
1. Run `sh run.sh`

Design information can be found at pubsub\_admin\_udp\_mc/README.md

### Running PSA ZMQ

For ZeroMQ without encryption, skip the steps 1-12 below

1. Run `touch ~/pubsub.keys`
1. Run `echo "aes_key:{AES_KEY here}" >> ~/pubsub.keys`. Note that AES_KEY is just a sequence of random bytes. To generate such a key, you can use the command " cat /dev/urandom | hexdump -v -e '/1 "%02X"' | head -c 32"(this will take out of /dev/urandom 16 bytes, thus a 128bit key)
1. Run `echo "aes_iv:{AES_IV here}" >> ~/pubsub.keys`.  Note that AES_IV is just a sequence of random bytes. To generate such an initial vector , you can use the command " cat /dev/urandom | hexdump -v -e '/1 "%02X"' | head -c 16"(this will take out of /dev/urandom 8 bytes, thus a 64bit initial vector) 
1. Run `touch ~/pubsub.conf`
1. Run `echo "keys.file.path=$HOME" >> ~/pubsub.conf`
1. Run `echo "keys.file.name=pubsub.keys" >> ~/pubsub.conf`
1. To generate ZMQ keypairs
1. Run `pubsub/keygen/makecert cert_topic1.pub cert_topic1.key`
1. To encrypt files
1. Run `pubsub/keygen/ed_file ~/pubsub.keys cert_topic1.key cert_topic1.key.enc`
1. Store the keys in the pubsub/examples/keys/ directory
1. Build project to include these keys
1. Add to the config.properties the property SECURE_TOPICS=<list_of_secure_topics> 

For ZeroMQ without encryption, start here

1. Run `etcd`
1. Open second terminal on pubsub root
1. Run `cd deploy/pubsub/pubsub_publisher_zmq`
1. Run `cat ~/pubsub.conf >> config.properties` (only for ZeroMQ with encryption)
1. Run `sh run.sh`
1. Open third terminal on pubsub root
1. Run `cd deploy/pubsub/pubsub_subscriber_zmq`
1. Run `cat ~/pubsub.conf >> config.properties` (only for ZeroMQ with encryption)
1. Run `sh run.sh`
