## Deployment Admin

The Celix Deployment Admin implements the OSGi Deployment Admin specification, which provides functionality to manage deployment packages. Deployment package are bundles and other artifacts that can be installed, updated and uninstalled as single unit.

It can be used for example with Apache Ace, which allows you to centrally manage and distribute software components, configuration data and other artifacts.

###### Properties
                  tags used by the deployment admin

## CMake option
    BUILD_DEPLOYMENT_ADMIN=ON

## Deployment Admin Config Options

- deployment_admin_identification     id used by the deployment admin to identify itself
- deployment_admin_url                url of the deployment server
- deployment_cache_dir                possible cache dir for the deployment admin update
- deployment_tags

## Using info

If the Celix Deployment Admin is installed The `FindCelix.cmake` will set:
 - The `Celix::deployment_admin_api` interface (i.e. headers only) library target
 - The `Celix::deployment_admin` bundle target
