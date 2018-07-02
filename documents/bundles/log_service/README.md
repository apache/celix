# Log Service

The Celix Log Service realizes an adapted implementation of the OSGi Compendium Log Service. This is a very simple implementation which only stores the log in memory. It can be combined with one of the available Log Writers to forward the buffered entries to e.g. stdout or syslog.

To ease the use of the Log Service, the [Log Helper](public/include/log_helper.h) can be used. It wraps and therefore simplifies the log service usage.

## Properties
    LOGHELPER_ENABLE_STDOUT_FALLBACK      If set to any value and in case no Log Service is found the logs
                                          are still printed on stdout. 

## CMake option
    BUILD_LOG_SERVICE=ON

## Using info

If the Celix Log Service is installed, 'find_package(Celix)' will set:
 - The `Celix::log_service_api` interface (i.e. header only) library target
 - The `Celix::log_service` bundle target
 - The `Celix::log_helper` static library target
