#{{
#import yaml
#bundle = None 
#with open(bundleFile) as input :
#	bundle = yaml.load(input)
#
#if not 'components' in bundle or bundle['components'] is None:
# 	bundle['components'] = []
#
#}}
#{{end}}


SET(
	BUNDLE_NAME 
#{{ 
#cog.outl("\t\"%s\"" % bundle['name']) 
#}}
"mybundle" #do not edit, generated code
#{{end}}
)

SET(
	BUNDLE_SYMBOLICNAME 
#{{ 
#cog.outl("\t\"%s\"" % bundle['symbolicName']) 
#}}
"org.example.mybundle" #do not edit, generated code
#{{end}}
)

SET(BUNDLE_VERSION "0.0.1")

include_directories(
	"private/include"
	"public/include"
)

bundle(
#{{ 
#cog.outl("\t%s SOURCES" % bundle['name']) 
#cog.outl("\t\tprivate/src/bundle_activator")
#for comp in bundle['components'] :
#	cog.outl("\t\tprivate/src/%s" % comp['name'])
#}}
mybundle SOURCES #do not edit, generated code
	private/src/bundle_activator #do not edit, generated code
	private/src/example #do not edit, generated code
#{{end}}
)


#{{ 
#cog.outl("SET(BUNDLE_LIB \"%s\")" % bundle['name']) 
#}}
SET(BUNDLE_LIB "mybundle") #do not edit, generated code
#{{end}}

target_link_libraries(${BUNDLE_LIB} ${CELIX_FRAMEWORK_LIBRARY} ${CELIX_UTILS_LIBRARY})
