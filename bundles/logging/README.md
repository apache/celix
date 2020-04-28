# Log Service

The Celix Log Service realizes an adapted implementation of the OSGi Compendium Log Service. This is a very simple implementation which only stores the log in memory. It can be combined with one of the available Log Writers to forward the buffered entries to e.g. stdout or syslog.

To ease the use of the Log Service, the [Log Helper]( loghelper_include/log_helper.h) can be used. It wraps and therefore simplifies the log service usage.

## Logging Properties
Properties shared among the logging bundles

    CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL The default active log level for created log services. Default is "info".

## Log Admin Properties
Properties specific for the Celix Log Admin (`Celix::log_admin` bundle)

    CELIX_LOG_ADMIN_FALLBACK_TO_STDOUT If set to true, the log admin will log to stdout/stderr if no celix log writers are available. Default is true
    CELIX_LOG_ADMIN_ALWAYS_USE_STDOUT If set to true, the log admin will always log to stdout/stderr after forwaring log statements to the available celix log writers. Default is false.
    CELIX_LOG_ADMIN_LOG_SINKS_DEFAULT_ENABLED Whether discovered log sink are default enabled. Default is true.
    
## CMake option
    BUILD_LOG_SERVICE=ON

## Using info

If the Celix Log Service is installed, 'find_package(Celix)' will set:
 - The `Celix::log_service_api` interface (i.e. header only) library target (v2 and v3 api)
 - The `Celix::log_admin` bundle target. The bundle contaiting th 
 - The `Celix::log_helper` static library target. Helper library with common logger functionality and helpers to setup logging
 
Also the following deprecated bundle will be set:
 - The `Celix::log_helper` static library target. Helper library to setup loggers. Deprecated, use Celix::log_helper instead.
 - The `Celix::log_service` bundle target. The log service bundle. Deprecated, use Celix::log_admin instead.
 - TODO deprecated log writers