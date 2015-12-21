#!/usr/bin/env python
import os
import argparse
from . import generators

#TODO add support to add licence text to all created files
#TODO add support to select pthread or celix_threads

def main() :
	parser = argparse.ArgumentParser("celix-bootstrap")

	CREATE_PROJECT_COMMAND = "create_project"	
	CREATE_BUNDLE_COMMAND = "create_bundle"
	CREATE_SERVICE_COMMAND = "create_services"

	UPDATE_COMMAND = "update"

	#positional
	parser.add_argument("command", help="The command to executee", choices=(CREATE_PROJECT_COMMAND, CREATE_BUNDLE_COMMAND, CREATE_SERVICE_COMMAND, UPDATE_COMMAND))
	parser.add_argument("directory", help="The directory to work on")

	#optional
	parser.add_argument("-e", "--erase", help="erase cog code when updating", action="store_true")
	parser.add_argument("-t", "--template_dir", help="define which template directory to use. Default is '%s/templates'" % os.path.dirname(__file__), action="store")

	args = parser.parse_args()
	
	gm = GeneratorMediator(args.directory, args.erase, args.template_dir)

	if args.command == CREATE_BUNDLE_COMMAND :
		gm.createBundle()
	elif args.command == CREATE_PROJECT_COMMAND :
		gm.createProject()
	elif args.command == CREATE_SERVICE_COMMAND :
		gm.createServices()
	elif args.command == UPDATE_COMMAND :
		gm.update()
	else :
		sys.exit()


class GeneratorMediator :
	
	gendir = None
	bundleGenerator = None
	projectGenerator = None
	servicesGenerator = None
	
	def __init__(self, gendir, erase, template_dir) :		
		self.gendir = gendir
		self.bundleGenerator = generators.Bundle(gendir, erase, template_dir)
		self.projectGenerator = generators.Project(gendir, erase, template_dir)
		self.servicesGenerator = generators.Services(gendir, erase, template_dir)

	def createBundle(self) :
		self.bundleGenerator.create()
	
	def createProject(self) :
		self.projectGenerator.create()	
	
	def createServices(self) :
		self.servicesGenerator.create()

	def update(self) :
		if os.path.isfile(os.path.join(self.gendir, "bundle.yaml")) :
			print("Generating/updating bundle code")
			self.bundleGenerator.update()
		if os.path.isfile(os.path.join(self.gendir, "project.yaml")) :
			print("Generating/updating project code")
			self.projectGenerator.update()
		if os.path.isfile(os.path.join(self.gendir, "services.yaml")) :
			print("Generating/updating services code")
			self.servicesGenerator.update()
