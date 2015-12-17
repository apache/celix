## Remote Services

Celix Remote Services contains several implementations of the OSGi Remote Services and Enterprise Remote Service Admin Service Specification:

* **Remote Service Admin** implements functionality to import/export services through a set of configuration types. The Remote Service Admin service is a passive Distribution Provider, not taking any action to export or import itself.
* **Topology Manager** provides the policy for importing and exporting services
through the Remote Service Admin service.
* **Discovery** provides detection of exported services through some discovery protocol.
