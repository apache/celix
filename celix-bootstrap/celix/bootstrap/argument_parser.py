#!/usr/bin/env python
import os
import argparse
from . import generators
import subprocess

def main() :
	parser = argparse.ArgumentParser("celix-bootstrap")

	CREATE_PROJECT_COMMAND = "create_project"	
	CREATE_BUNDLE_COMMAND = "create_bundle"

	UPDATE_COMMAND = "update"
	BUILD_COMMAND = "build"

	#positional
	parser.add_argument("command", help="The command to execute", choices=(CREATE_PROJECT_COMMAND, CREATE_BUNDLE_COMMAND, UPDATE_COMMAND, BUILD_COMMAND))
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
	elif args.command == UPDATE_COMMAND :
		gm.update()
	elif args.command == BUILD_COMMAND :
		gm.build()
	else :
		sys.exit()


class GeneratorMediator :
	
	gendir = None
	bundleGenerator = None
	projectGenerator = None
	
	def __init__(self, gendir, erase, template_dir) :		
		self.gendir = gendir
		self.bundleGenerator = generators.Bundle(gendir, erase, template_dir)
		self.projectGenerator = generators.Project(gendir, erase, template_dir)

	def createBundle(self) :
		self.bundleGenerator.create()
	
	def createProject(self) :
		self.projectGenerator.create()	
	

	def update(self) :
		if os.path.isfile(os.path.join(self.gendir, "bundle.yaml")) :
			print("Generating/updating bundle code")
			self.bundleGenerator.update()
		if os.path.isfile(os.path.join(self.gendir, "project.yaml")) :
			print("Generating/updating project code")
			self.projectGenerator.update()

	def build(self) :
		if not os.path.exists(self.gendir):
			print("Creating Directory " + self.gendir)
			os.makedirs(self.gendir)

		cmake = subprocess.Popen(["cmake", os.getcwd()], cwd=self.gendir, stderr=subprocess.STDOUT)
		if cmake.wait() != 0:
			print("Cmake run failed. Do you perform to need an update run first?")
		else:
			make = subprocess.Popen(["make", "all", "deploy"], cwd=self.gendir, stderr=subprocess.STDOUT)
			if make.wait() != 0:
				print("Compilation failed. Please check your code")
