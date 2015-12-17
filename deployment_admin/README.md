## Deployment Admin

The Celix Deployment Admin implements the OSGi Deployment Admin specification, which provides functionality to manage deployment packages. Deployment package are bundles and other artifacts that can be installed, updated and uninstalled as single unit.

It can be used for example with Apache Ace, which allows you to centrally manage and distribute software components, configuration data and other artifacts.

###### Properties
    deployment_admin_identification     id used by the deployment admin to identify itself
    deployment_admin_url                url of the deployment server
    deployment_cache_dir                possible cache dir for the deployment admin update
    deployment_tags                     tags used by the deployment admin

###### CMake option
    BUILD_DEPLOYMENT_ADMIN=ON
