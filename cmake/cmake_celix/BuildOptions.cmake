
option(ENABLE_ADDRESS_SANITIZER "Enabled building with address sanitizer. Note for gcc libasan must be installed" OFF)

if (ENABLE_ADDRESS_SANITIZER)
    set(CMAKE_C_FLAGS "-lasan -fsanitize=address ${CMAKE_C_FLAGS} -Wl,--allow-shlib-undefined")
    set(CMAKE_CXX_FLAGS "-lasan -fsanitize=address ${CMAKE_CXX_FLAGS} -Wl,--allow-shlib-undefined")
endif()
