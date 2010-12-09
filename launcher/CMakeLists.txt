add_executable(launcher launcher.c)

include_directories("${PROJECT_SOURCE_DIR}/celix")
target_link_libraries(launcher framework)

install(TARGETS launcher RUNTIME DESTINATION bin COMPONENT framework)