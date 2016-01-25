## Topology Manager

The Topology Manager decides which services should be imported and exported according to a defined policy. Currently, only one policy is implemented in Celix, the *promiscuous* policy, which simply imports and exports all services. Note that the Topology Manager is essential to use remote services.

###### CMake option
    BUILD_RSA_TOPOLOGY_MANAGER=ON
