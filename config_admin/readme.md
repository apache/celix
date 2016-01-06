# Configuration Admin

---

## Introduction
The configuration Admin service allows defining and deploying configuration data to bundles.
When compared to config.properties it adds the option to update configuration data by providing a persisten storage. It also allows changing configuration data at run-time.

---

## Design

The config_admin bundle implements the configuration_admin service, the interface to configuration objects and the interface of a managed service. At the moment, the implementation uses a config_admin_factory to generate config_admin services for each bundle that wants to use this service. This is an inheritance of the original design and not needed.

---

## TODO

1. Test the configuration of a service_factory
2. Think about the option to allow remote update of the managed_services

---

## Usage

1. Bundle that needs configuration data
   This bundle has to register next to its normal service a managed service that has an update method.  Use config_admin_tst/example_test as an example (it is better than example_test2)
2. Bundle/application that wants to update the configuration data of the system
   This bundle needs to retrieve the running config_admin service. With this service it can retrieve all configuration objects for all known Persistent Identifiers (PIDs). For each PID, get all properites that need to be updated. See config_admin_test for an example.
