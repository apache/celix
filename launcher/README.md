## Launcher

The Celix Launcher is a generic executable for launching the Framework. It reads a java properties based configuration file.

The Launcher also passes the entire configuration to the Framework, this makes them available to the bundleContext_getProperty function.

###### Properties

    cosgi.auto.start.1                  Space delimited list of bundles to install and start when the
                                        Launcher/Framework is started. Note: Celix currently has no
                                        support for start levels, even though the "1" is meant for this.
    org.osgi.framework.storage          sets the bundle cache directory
    org.osgi.framework.storage.clean    If set to "onFirstInit", the bundle cache will be flushed
                                        when the framework starts

###### CMake option
    BUILD_LAUNCHER=ON
