import shutil
import os
import sys
import yaml
import cogapp

class BaseGenerator(object):
	gendir = None
	descriptor = None
	erase_cog = False
	template_dir = None
	profile= None
	gen_code_suffix = "do not edit, generated code"
	markers="{{ }} {{end}}"

	def __init__(self, gendir, profile, erase, template_dir) :
		self.gendir = gendir
		self.descriptor = "%s/%s.yaml" % (gendir, profile)
		self.template_dir = os.path.join(os.getcwd(), os.path.dirname(__file__), "templates")
		self.profile = profile
		self.erase_cog = erase
		if template_dir is not None :
			self.template_dir = template_dir

	def set_erase_cog(self, erase) :
		self.erase_cog = erase

	def set_template_dir(self, tdir) :
		self.template_dir = tdir

	def set_gen_cod_suffic(self, suffix) :
		self.gen_code_suffic = suffix

	def set_cog_markers(markers) :
		self.markers = markers
		
	def create(self) :
		if os.path.exists(self.descriptor) :
			print("%s Already exists. Will not override existing %s.yaml" % (self.descriptor, self.profile))
		else :
			if not os.path.exists(self.gendir) :
				os.makedirs(self.gendir)
			shutil.copyfile(os.path.join(self.template_dir, self.profile, "%s.yaml" % self.profile), self.descriptor)
			
			print("Edit the %s file and run 'celix-bootstrap update %s' to generate the source files" % (self.descriptor, self.gendir))

	def read_descriptor(self) :
		if os.path.isdir(self.gendir) and os.path.exists(self.descriptor) :
			with open(self.descriptor) as inputFile :
				try :
					return yaml.load(inputFile)
				except ValueError as e:
					print("ERROR: %s is not a valid yaml file: %s" % (self.descriptor, e))
					sys.exit(1)

	def update_file(self, template, targetFile, options=[], commentsPrefix="//") :
		print("Creating file %s %s" % (self.gendir, targetFile))
		cog_file = os.path.join(self.gendir, targetFile)
#		cog_file = os.path.join('.', targetFile)
		if not os.path.exists(cog_file) :
			print("Creating file %s" % cog_file)
			if not os.path.exists(os.path.dirname(cog_file)) :
				os.makedirs(os.path.dirname(cog_file))
			template_abs = os.path.join(self.template_dir, self.profile,  template)
			shutil.copyfile(template_abs, cog_file)
	
		backup_cog_file = "%s/.cog/%s" % (self.gendir, targetFile)
		if self.erase_cog :
			if os.path.exists(backup_cog_file) :
				print("Will not erase cog code, backup file (%s) for source already exists." % backup_cog_file)
				sys.exit(1)
			else :
				if not os.path.exists(os.path.dirname(backup_cog_file)) :
					os.makedirs(os.path.dirname(backup_cog_file))
				shutil.copy(cog_file, backup_cog_file)
		
		cog = cogapp.Cog()
		cog_options = ["cog", "--markers=%s" % self.markers] + options 
		if self.erase_cog :
			cog_options += ["-d", "-o", cog_file, backup_cog_file]
		else :
			cog_options += ["-r", "-e" ]
			if commentsPrefix is not None: 
				cog_options += [ "-s", " %s%s" %(commentsPrefix, self.gen_code_suffix)]
			cog_options += [cog_file]

		cog.main(cog_options)
	

class Bundle(BaseGenerator):

	def __init__(self, gendir, erase, template_dir) :
		BaseGenerator.__init__(self, gendir, "bundle", erase, template_dir)
		#python3 super(Bundle, self).__init__(gendir, "bundle")

	def update_cmakelists(self) :
		options = ["-D", "bundleFile=%s" % self.descriptor]
		self.update_file("CMakeLists.txt", "CMakeLists.txt", options, "#")	
		
	def update_deploy_file(self) :
		options = ["-D", "bundleFile=%s" % self.descriptor]
		self.update_file("deploy.cmake", "deploy.cmake", options, "#")	

	def update_activator(self) :
		options = ["-D", "bundleFile=%s" % self.descriptor]
		self.update_file("bundle_activator.c", "private/src/bundle_activator.c", options)

	def update_component_header(self, componentName) :
		genfile = "private/include/%s.h" % componentName
		options = ["-D", "bundleFile=%s" % self.descriptor, "-D", "componentName=%s" % componentName]
		self.update_file("component.h", genfile, options)

	def update_component_source(self, componentName) :
		genfile = "private/src/%s.c" % componentName
		options = ["-D", "bundleFile=%s" % self.descriptor, "-D", "componentName=%s" % componentName]
		self.update_file("component.c", genfile, options)

	def update_service_header(self, componentName, service) :
		genfile = "public/include/%s" % service['include']
		options = ["-D", "bundleFile=%s" % self.descriptor,  "-D", "componentName=%s" % componentName, "-D", "serviceName=%s" % service['name']]
		self.update_file("service.h", genfile, options)

	def create(self) :
		self.update_file(os.path.join(self.template_dir, self.profile, "%s.yaml" % self.profile), "%s.yaml" % self.profile, [], None)

	def update(self) :
		bd = self.read_descriptor()
		if bd is None :
			print("%s does not exist or does not contain a bundle.yaml file" % self.gendir)
		else :
			self.update_cmakelists()	
			self.update_deploy_file()
			self.update_activator()
			if 'components' in bd and bd['components'] is not None:
				for comp in bd['components'] :
					self.update_component_header(comp['name'])
					self.update_component_source(comp['name'])
					if 'providedServices' in comp and comp['providedServices'] is not None:
						for service in comp['providedServices']:
							self.update_service_header(comp['name'], service)

class Project(BaseGenerator):

	def __init__(self, gendir, erase, template_dir) :
		BaseGenerator.__init__(self, gendir, "project", erase, template_dir)
		#python3 super(Project, self).__init__(gendir, "project")

	def update_cmakelists(self) :
                options = ["-D", "projectFile=%s" % self.descriptor]
                self.update_file("CMakeLists.txt", "CMakeLists.txt", options, "#")      

        def update_deploy_file(self) :
                options = ["-D", "projectFile=%s" % self.descriptor]
                self.update_file("deploy.cmake", "deploy.cmake", options, "#")  

        def create(self) :
                self.update_file(os.path.join(self.template_dir, self.profile, "%s.yaml" % self.profile),  "%s.yaml" % self.profile, [], None)  

	def update(self) :
		descriptor = self.read_descriptor()
		if descriptor is None :
                        print("%s does not exist or does not contain a project.yaml file" % self.gendir)
                else :
                        self.update_cmakelists()        
                        self.update_deploy_file()
