#Apache Celix - CMake Commands

For Apache Celix several cmake command are added to be able to work with Apache Celix bundles and deployments.

#Bundles

##add_bundle
Add a bundle to the project.  There are three variants:
- With SOURCES the bundle will be created using a list of sources files as input for the bundle activator library.
- With ACTIVATOR the bundle will be created using the library target or absolute path to existing library as activator library.
- With no SOURCES or ACTIVATOR a bundle without a activator will be created.

```CMake
add_bundle(<bundle_target_name> 
    SOURCES source1 source2 ...
    [NAME bundle_name] 
    [SYMBOLIC_NAME bundle_symbolic_name]
    [DESCRIPTION bundle_description]
    [VERSION bundle_version]
    [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
    [EXPORT_LIBRARIES export_lib1 export_lib2 ...]
    [IMPORT_LIBRARIES import_lib1 import_lib2 ...]
    [HEADERS "header1: header1_value" "header2: header2_value" ...]
)
```

```CMake
add_bundle(<bundle_target_name> 
    ACTIVATOR <activator_lib>
    [NAME bundle_name] 
    [SYMBOLIC_NAME bundle_symbolic_name]
    [DESCRIPTION bundle_description]
    [VERSION bundle_version]
    [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
    [EXPORT_LIBRARIES export_lib1 export_lib2 ...]
    [IMPORT_LIBRARIES import_lib1 import_lib2 ...]
    [HEADERS "header1: header1_value" "header2: header2_value" ...]
)
```

```CMake
add_bundle(<bundle_target_name> 
    [NAME bundle_name] 
    [SYMBOLIC_NAME bundle_symbolic_name]
    [DESCRIPTION bundle_description]
    [VERSION bundle_version]
    [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
    [EXPORT_LIBRARIES export_lib1 export_lib2 ...]
    [IMPORT_LIBRARIES import_lib1 import_lib2 ...]
    [HEADERS "header1: header1_value" "header2: header2_value" ...]
)
```

- If NAME is provided that will be used as Bundle-Name. Default the bundle target name is used as symbolic name.
- If SYMBOLIC_NAME is provided that will be used as Bundle-SymbolicName. Default the bundle target name is used as symbolic name.
- If DESCRIPTION is provided that will be used as Bundle-Description. Default this is empty
- If VERSION is provided. That will be used for the Bundle-Version. In combination with SOURCES the version will alse be use to set the activator library target property VERSION and SOVERSION.
For SOVERSION only the major part is used. Expected scheme is "<major>.<minor>.<path>". Default version is "0.0.0"
If PRIVATE_LIBRARIES is provided all provided lib are added to the "Private-Library" manifest statement and added in the root of the bundle. libraries can be cmake library targets or absolute paths to existing libraries.  
- If EXPORT_LIBRARIES is provided all provided lib are added to the "Export-Library" manifest statement and added in the root of the bundle. libraries can be cmake library targets or absolute paths to existing libraries. This is not yet supported by the celix framework.
- If EXPORT_LIBRARIES is provided all provided lib are added to the "Import-Library" manifest statement and added in the root of the bundle. libraries can be cmake library targets or absolute paths to existing libraries.  This is not yet supported by the celix framework
- If HEADERS is provided the headers values are appended to the bundle manifest.

##bundle_private_libs
Add libraries to a bundle target. The libraries should be cmake library targets or an absolute path to a existing library.

```CMake
bundle_private_libs(<bundle_target>
    lib1 lib2 ...
)
```

##bundle_files
Add files to the target bundle. DESTINATION is relative to the bundle archive root. 
The rest of the command is conform file(COPY ...) cmake command.
See cmake file(COPY ...) command for more info.

```CMake
bundle_files(<bundle_target>
    files... DESTINATION <dir>
    [FILE_PERMISSIONS permissions...]
    [DIRECTORY_PERMISSIONS permissions...]
    [NO_SOURCE_PERMISSIONS] [USE_SOURCE_PERMISSIONS]
    [FILES_MATCHING]
    [[PATTERN <pattern> | REGEX <regex>]
    [EXCLUDE] [PERMISSIONS permissions...]] [...])
)
```


##bundle_headers
Append the provided headers to the target bundle manifest.

```CMake
bundle_headers(<bundle_target>
    "header1: header1_value"
    "header2: header2_value"
    ...
)
```

##bundle_symbolic_name
Set bundle symbolic name

```CMake
bundle_symbolic_name(<bundle_target> symbolic_name)
```

##bundle_name
Set bundle name

```CMake
bundle_name(<bundle_target> name)
```

##bundle_version
Set bundle version

```CMake
bundle_version(<bundle_target> version)
```

##bundle_description
Set bundle description

```CMake
bundle_description(<bundle_target> description)
```

##install_bundle
Install bundle when 'make install' is executed. 
Bundles are installed at `<install-prefix>/share/<project_name>/bundles`.
Headers are installed at `<install-prefix>/include/<project_name>/<bundle_name>`
Resources are installed at `<install-prefix>/shared/<project_name>/<bundle_name>`

```CMake
install_bundle(<bundle_target>
    [PROJECT_NAME] project_name
    [BUNDLE_NAME] bundle_name
    [HEADERS header_file1 header_file2 ...]
    [RESOURCES resource1 resource2 ...]
)
```

- If PROJECT_NAME is provided that will be used as project name for installing. Default is the cmake project name.
- If BUNDLE_NAME is provided that will be used as bundle for installing the headers. Default is the bundle target name.
- If HEADERS is provided the list of provided headers will be installed.
- If RESOURCES is provided the list of provided resources will be installed.

#Deployments

#add_deploy
Add a deployment, consisting out of a selection of bundles, for the project. 
Deployments can be used to run/test a selection of bundles in the celix framework.
A deployment can be found in `<cmake_build_dir>/deploy[/<group_name>]/<deploy_name>`. 
Use the run.sh to run the deployments.

```CMake
add_deploy(<deploy_target_name>
    [COPY] 
    [GROUP group_name]
    [NAME deploy_name]
    [BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
)
```

The provided bundle targets for a deployment do not have to exists (yet).
This removes the need for correctly ordening the add_bundle commands so that all bundle target are present before an add_deploy command.
If the bundle target is never added CMake will give an error:
```
  Error evaluating generator expression:

    $<TARGET_PROPERTY:foo,BUNDLE_FILE>
```

- If COPY is provided the selected bundles will be copied in a bundles dir and the generated config.properties will use relative paths to the bundle locations. Default bundles will not be copied and the generated config.properties will use absolute references to the bundle locations.
- If GROUP is provided the deployment will be grouped in the provided group name. 
- If NAME is provided that name will be used for the deployment dir. Default the deploy target name will be used.
- If BUNDLES is provided the list of bundles will be added the the generated config.properties for startup. Combined with COPY the bundles will also be copied to a bundles dir.
- If PROPERTIES is provided the list of properties will be appended to the generated config.properties

#deploy_bundles_dir
Deploy a selection of bundles to the provided bundle dir. This can be used to create an endpoints / proxies bundles dir for the remote service admin or drivers bundles dir for the device access. 

```CMake
deploy_bundles_dir(<deploy_target_name>
    DIR_NAME dir_name
    BUNDLES bundle1 bundle2 ...
)
```

#deploy_bundles
Deploy the selected bundles. The bundles are configured for auto starting. 

```CMake
deploy_bundles(<deploy_target_name>
    bundle1 bundle2 ...
)
```

#deploy_properties

```CMake
Add the provided properties to the target deploy config.properties.

deploy_properties(<deploy_target_name>
    "prop1=val1" "prop2=val2" ...
)

