# Etcdlib
etcd is a distributed, consistent key-value store for shared configuration and service discovery, part of the CoreOS project.

This repository provides a library for etcd for C applications.

Etcdlib can be used as part of Celix but is also useable stand-alone.

## Preparing
The following packages (libraries + headers) should be installed on your system:

*	Development Environment
	*	build-essentials (gcc/g++ or clang/clang++)
	*	cmake (3.2 or higher)

* 	Etcdlib Dependencies
	*	curl
	*	jansson

## Download the Apache Celix sources
To get started you first have to download the Apache Celix sources. This can be done by cloning the Apache Celix git repository:

```bash
# Create a new workspace to work in, e.g:
mkdir -p ${HOME}/workspace
export WS=${HOME}/workspace
cd ${WS}

# clone the repro
git clone --single-branch --branch develop https://github.com/apache/celix.git
```

## Building
Etcdlib uses [CMake](https://cmake.org) as build system. CMake can generate (among others) makefiles or ninja build files. Using ninja build files will result in a faster build.

### Building using CMake and makefiles:
```bash
cd ${WS}/celix/etcdlib
mkdir build
cd build
cmake .. 
make 
```

### Building using CMake and Ninja
```bash
cd ${WS}/celix/etcdlib
mkdir build
cd build
cmake -G Ninja ..
ninja
```

## Editing Build options
With use of CMake, Etcdlib makes it possible to edit build options. This enabled users, among other options, to configure a install location.
To edit the options use ccmake or cmake-gui. For cmake-gui an additional package install can be necessary (Fedora: `dnf install cmake-gui`). 

```bash
cd ${WS}/celix/etcdlib/build
ccmake .
# Edit the CMAKE_INSTALL_PREFIX config to set the install location
```

For this guide we assume the CMAKE_INSTALL_PREFIX is `/usr/local`.

## Installing Etcdlib

```bash
cd ${WS}/celix/etcdlib/build
make
sudo make install
```
