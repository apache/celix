#{{
#import fnmatch
#import os
#import yaml
#
#project = None 
#bundle = None 
#
#with open(projectFile) as input :
#	project = yaml.load(input)
#
#cog.outl("deploy( \"%s\" BUNDLES" % project['name'])
#cog.outl("\t${CELIX_BUNDLES_DIR}/shell.zip")
#cog.outl("\t${CELIX_BUNDLES_DIR}/dm_shell.zip")
#cog.outl("\t${CELIX_BUNDLES_DIR}/shell_tui.zip")
#cog.outl("\t${CELIX_BUNDLES_DIR}/log_service.zip")
#
#
#for root, dirs, filenames in os.walk('.'):
#	for foundFile in fnmatch.filter(filenames, 'bundle.yaml'):
#		bundleFile = root + '/' + foundFile
#		with open(bundleFile) as input :
#                       bundle = yaml.load(input)
#                       cog.outl("\t%s" % bundle['name'])
#
#cog.outl(")");
#}}
#{{end}}
