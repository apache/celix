# Log Writer

The Celix Log Writers are components that read/listen to the Log Service and print the Log entries to the console or syslog, respectively.

## CMake options
    BUILD_LOG_WRITER=ON
    BUILD_LOG_WRITER_SYSLOG=ON

## Using info

If the Celix Log Writers are installed `find_package(CELIX)` will set:
 - The `Celix::log_writer_stdout` bundle target
 - The `Celix::log_writer_syslog` bundle target
