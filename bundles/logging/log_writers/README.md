---
title: Log Writers
---

# Log Writer

The Celix Log Writers are components that sinks log from the Celix log service to different backends.

## CMake options
    BUILD_SYSLOG_WRITER=ON

## Using info

If the Celix Log Writers are installed `find_package(CELIX)` will set:
 - The `Celix::syslog_writer` bundle target
