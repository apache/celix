add_library(dependency_manager STATIC dependency_manager.c dependency_activator_base.c service_component.c service_dependency.c)

include_directories("${PROJECT_SOURCE_DIR}/celix")
target_link_libraries(dependency_manager framework)