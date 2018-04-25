# TODO integrate cxx dep man into framework

- Move documentation
- Move dummy targets
- Depecrate the Celix::dependency_manager_cxx_static target. (how?)
- Eventually remove the deprecated cxx dm target
- The bundle activator is now still a small .cc file, still resulting in
a static libary which has to be linked as whole. Make this a src dependency? or some how a
header impl ?