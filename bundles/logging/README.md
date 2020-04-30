# Celix Logging Facilities

The Celix Logging facility is service oriented and logging technology agnostic logging solution.

Bundles can request (services on demand) and use `celix_log_service_t` services to log events.
Logging support the following log levels: `trace`, `debug`, `info`, `error`, `fatal`. 

Bundles can provide `celix_log_sink_t` services to sink log message to different logging backends (e.g. syslog, log4c, etc)

The `Celix::log_admin` bundle facilitates the `celix_log_service_t` services and 'connects' these to the available 
`celix_log_sink_t` services. If there is no `celix_log_sink_t` service available, log messages will be
printed on stdout/stderr.

The Celix shell command `celix::log_admin` can be used to view the existing log services and sinks,
changed the active log level per log services and enable/disable log sinks.
For example:
- `celix::log_admin` list the available log services and log sinks.
- `celix::log_admin log error` Set the active log level for all log services to `error`.
- `celix::log_admin log celix_ trace` Set the active log level for all log services starting with 'celix_' to `trace`.
- `celix::log_admin sink false` Disables all available log sinks.
- `celix::log_admin sink celix_syslog true` Enables all log sinks starting with 'celix_syslog'.

The `Celix::log_helper` static library can be used to more easily request a `celix_log_service_t`. 
An additional benefit of the `Celix:log_helper` is that if the `Celix::log_admin` is not installed, 
log messages will be printed on stdout/stderr.


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
 - The `Celix::log_admin` bundle target. The log admin will create log services on demand and forward log message to the available log sinks. 
 - The `Celix::log_helper` static library target. Helper library with common logger functionality and helpers to setup logging
 - The `Celix::log_writer_syslog` bundle target. A bundle which provides a `celix_log_sink_t` service for syslog.
 
Also the following deprecated bundle will be set:
 - The `Celix::log_service` bundle target. The log service bundle. Deprecated, use Celix::log_admin instead.
 - The `Celix::log_writer_stdout` bundle target. Deprecated bundle. Logging to stdout is now an integral part of the log admin.