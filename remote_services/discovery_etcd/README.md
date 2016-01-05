## Discovery ETCD

The Celix Discovery ETCD bundles realizes OSGi services discovery based on [etcd](https://github.com/coreos/etcd).

###### Properties
    DISCOVERY_ETCD_ROOT_PATH             used path (default: discovery)
    DEFAULT_ETCD_SERVER_IP               ip address of the etcd server (default: 127.0.0.1)
    DEFAULT_ETCD_SERVER_PORT             port of the etcd server  (default: 2379)
    DEFAULT_ETCD_TTL                     time-to-live for etcd entries in seconds (default: 30)

###### CMake option
    BUILD_RSA_DISCOVERY_ETCD=ON
