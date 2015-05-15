#{{
#import json
#bundle = None 
#with open(bundleFile) as input :
#	bundle = json.load(input)
#cog.outl("deploy( \"%s\" BUNDLES" % bundle['name'])
#cog.outl("\t${CELIX_BUNDLES_DIR}/shell.zip")
#cog.outl("\t${CELIX_BUNDLES_DIR}/shell_tui.zip")
#cog.outl("\t${CELIX_BUNDLES_DIR}/log_service.zip")
#cog.outl("\t%s" % bundle['name'])
#}}
deploy( "mybundle" BUNDLES #do not edit, generated code
	${CELIX_BUNDLES_DIR}/shell.zip #do not edit, generated code
	${CELIX_BUNDLES_DIR}/shell_tui.zip #do not edit, generated code
	${CELIX_BUNDLES_DIR}/log_service.zip #do not edit, generated code
	mybundle #do not edit, generated code
#{{end}}
)
