## Device Access

The Device Access contains a for Celix adapted implementation of the OSGi Compendium Device Access Specification.

## Properties
    DRIVER_LOCATOR_PATH     Path to the directory containing the driver bundles, defaults to "drivers".
                            The Driver Locator uses this path to find drivers.

## CMake option
    BUILD_DEVICE_ACCESS=ON

## Using info

If the Celix Device Access is installed The `FindCelix.cmake` will set:
 - The `Celix::device_access_api` interface (i.e. headers only) library target
 - The `Celix::device_manager` bundle target
 - The `Celix::driver_locator` bundle target