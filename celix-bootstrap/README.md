# celix-bootstrap
A (python based) code generation tool for [Apache Celix](https://celix.apache.org/) projects and bundles.

##Installation

###Option 1 (install with distutils)

* Download Cog from https://pypi.python.org/pypi/cogapp
* Unpack and run `python setup.py install' in the cogapp directory
* Clone this repository
* Run `python setup.py install` in the celix-bootstrap directory

###Option 2 (configure with alias)
* Download Cog from https://pypi.python.org/pypi/cogapp
* Unpack it in a convenient directory 
* Clone this repository
* Create a celix-bootstrap alias:
```
alias celix-bootstrap='PYTHONPATH=${cog_dir}:${celix-boostrap_dir} python -m celix-bootstrap'
```

#Concept
The idea behind celix-boostrap is to enable users of Apache Celix to very fast setup of Apache Celix bundle projects. And after that to be able to generate and update the initial bundle code based on some declaritive attributes.

celix-boostrap is build on top of [Cog](https://celix.apache.org/). "Cog is a file generation tool. It lets you use pieces of Python code as generators in your source files to generate whatever text you need." 

The code generation code is added in the comments of the source code (in this case C or CMake syntax). This enables a
source files which contains manual code and generated code to coexists and to able to rerun code generation after manual editing source files.

Is good to known that you should not edit anything between the following comments.
```
{{
{{end}}
```

#Usage

You can setup a new Apache Celix project by executing following steps:
* Choose a project directory and cd to it
* Run `celix-bootstrap create_project` .
* Enter projectname and the location of the Apache Celix installation.
* Run `celix-bootstrap update .` to generate the needed files
* (Optional) Update the project.yaml file and run `celix-bootstrap update .` again to update the generated files
* (Optional) Run `celix-bootstrap update . -e` to update the project source files one more time and remove the code generation parts

You can create a new bundle by executing the following steps:
* cd to the project directory
* Run `celix-bootstrap create_bundle mybundle`
* Enter a bundlename and a symbolicName
* Confirm to create a component and enter a componentname
* Confirm whether the component should provide a service
* Enter include file, name, service name and type
* Confirm when the component should provide another service and provide the according details. Otherwise Decline.
* Confirm whether the component is dependent on another service
* Enter include file, name, service name and type and cardinality of the depending service
* Confirm when the component is dependent on another service and provide the according details. Otherwise Decline.
* Confirm if another component should be created. Otherwise Decline.
* Run `celix-bootstrap update mybundle` to generate the neccesary files
* Add the bundle to the project CMakeLists.txt file by adding the `add_subdirectory(mybundle)` line
* Add code to the component and when providing service also add the necesarry code to the bundle activator
* (Optional) Run `celix-bootstrap update . -e` to update the bundle source files one more time and remove the code generation parts

Additional Info
* You can use the -t argument to use a different set of template (project & bundle) files. The original template files can be found at ${celix-bootstrap_dir}/templates
