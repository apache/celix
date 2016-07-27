#Apache Celix - Building and Installing
Apache Celix aims to be support a broad range of UNIX platforms.
 
Currently the [continuous integration build server](https://travis-ci.org/apache/celix) build and tests Apache Celix for:

*   Ubuntu Trusty Tahr (14.04)
    * GCC 
    * CLang 
*   OSX
    * GCC 
    * CLang 
*   Android (cross-compiled on Ubuntu Trusty Tahr) 
    * GCC 

#Preparing 
The following packages (libraries + headers) should be installed on your system:

*	Development Environment
	*	build-essentials (gcc/g++ or clang/clang++) 
	* 	git
    *   java (for packaging bundles)
	*	cmake (3.2 or higher)
* 	Apache Celix Dependencies
	*	curl
	*	jansson
	*   libffi
	*   libxml2 (for remote services and bonjour shell)
	

For a debian based systems, the following should work:
<pre>
sudo apt-get install -yq --no-install-recommends \
	build-essential \
    ninja \ 
  	curl \
  	git \
  	libjansson-dev \
  	libcurl4-openssl-dev \
    java \
  	cmake \
  	libffi-dev \
  	libxml2-dev
</pre>

For Fedora based systems (dnf), the following should work:
<pre>
sudo dnf install \
    cmake \
    ninja-build \
    make \
    git \
    java \
    libcurl-devel \
    jansson-devel \
    libffi-devel \
    libxml2-devel
</pre>

For a OSX with brew installed, the following should work:
 <pre>
    brew update && \
    brew install lcov libffi cmake && \
    brew link --force libffi
 </pre>

##Download the Apache Celix sources
To get started you first have to download the Apache Celix sources. This can be done by cloning the Apache Celix git repository:

```bash
#Create a new workspace to work in, e.g:
mkdir ${HOME}/workspace
export WS=${HOME}/workspace
cd ${WS}

#clone the repro
git clone --single-branch --branch develop https://github.com/apache/celix.git
```

##Building Apache Celix
Apache Celix uses [CMake](https://cmake.org) as build system. CMake can generate (among others) makefiles or ninja build files. 
using ninja build files will result in a faster build.

###Building using CMake and makefiles:
```bash
cd ${WS}/celix
mkdir build
cd build
cmake .. 
make 
```

###Building using CMake and Ninja
```bash
cd ${WS}/celix
mkdir build
cd build
cmake -G Ninja ..
ninja
```

##Editing Build options
With use of CMake Apache Celix makes it possible to edit build options. This enabled users to configure a install location and selecting additional bundles.
To edit the options use ccmake or cmake-gui. For cmake-gui an additional package install can be neccesary (Fedora: `dnf install cmake-gui`). 

```bash
cd ${WS}/celix/build
ccmake .
#Edit options, e.g. enable BUILD_REMOTE_SHELL to build the remote (telnet) shell
#Edit the CMAKE_INSTALL_PREFIX config to set the install location
```

For this guide we assume the CMAKE_INSTALL_PREFIX is `/usr/local`.

##Installing Apache Celix

```bash
cd ${WS}/celix/build
make
sudo make install
```

##Running Apache Celix

If Apache Celix is succesfully installed running
```bash
celix
```
should give the following output:
"Error: invalid or non-existing configuration file: 'config.properties'.No such file or directory"

For more info how to build your own projects and/or running the Apache Celix examples see [Getting Started](../getting_started/readme.md)
