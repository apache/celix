#{{
#	import json
#	project = None 
#	with open(projectFile) as input :
#		project = json.load(input)
#}}
#{{end}}

cmake_minimum_required(VERSION 2.8)

#{{
#	cog.outl("project(%s C)" % project['name'])
#}}
project(myproject C) #do not edit, generated code
#{{end}}

#{{ 
#	cog.outl("set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} \"%s/share/celix/cmake/modules\")" % project['celix_install_dir'])
#}}
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/usr/local/share/celix/cmake/modules") #do not edit, generated code
#{{end}}

find_package(CELIX REQUIRED)
include_directories(${CELIX_INCLUDE_DIRS})

#TODO add sub directory for every bundle
#e.g. 
#add_subdirectory(mybundle)

deploy_targets()
