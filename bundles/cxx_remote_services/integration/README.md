# C++ Remote Service Amdatu Integration

This project part tests the integration of the C++ Remote Service Admin implementation with the 
C++ configuration-based remote service discovery. 

Because the C++ Remote Service Admin is based on export and import service factories and does not directly 
implement a transportation or serializer technology, the integration tests are based on a simple implementation of
inter process communication message queue (IPC mq) for transportation and a simple memcpy for serialization.
