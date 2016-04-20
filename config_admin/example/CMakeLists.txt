

SET(
	BUNDLE_NAME 
    "config_admin_example"
)

SET(
	BUNDLE_SYMBOLICNAME 
    "org.apache.celix.config_admin.example"
)

include_directories(
	"private/include"
	"public/include"
    ${PROJECT_SOURCE_DIR}/utils/public/include
    ../service/public/include
)

add_bundle(config_admin_example 
	VERSION 0.0.1
	SOURCES
		private/src/bundle_activator
		private/src/example
)

target_link_libraries(config_admin_example ${CELIX_FRAMEWORK_LIBRARY} ${CELIX_UTILS_LIBRARY})
