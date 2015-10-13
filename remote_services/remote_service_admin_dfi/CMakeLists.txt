

celix_subproject(RSA_REMOTE_SERVICE_ADMIN_DFI "Option to enable building the Remote Service Admin Service DFI" OFF)

if (RSA_REMOTE_SERVICE_ADMIN_DFI)

    find_package(CURL REQUIRED)
    find_package(Jansson REQUIRED)
    find_package(FFI REQUIRED)

    include_directories(
        ${CURL_INCLUDE_DIRS}
        ${JANSSON_INCLUDE_DIRS}
        ${FFI_INCLUDE_DIRS}
    )

    if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        include_directories(dynamic_function_interface/memstream)
    endif()

    add_subdirectory(dynamic_function_interface)
    add_subdirectory(rsa)

    if (ENABLE_TESTING)
        find_package(CppUTest REQUIRED)
        include_directories(${CPPUTEST_INCLUDE_DIR})
        add_subdirectory(dynamic_function_interface_tst)
        add_subdirectory(rsa_tst)
    endif()

endif()