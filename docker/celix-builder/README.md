# Intro

A celix-base is can be created in two steps. The reason behind these two steps is to be able to create small images only containing the 
really necessary files. For celix this means the celix executable and required libraries. To support debugging a gdb-server is also added.

The first docker image is responsible for building Celix and the required libaries, setting up a miminum image using dockerize and creating a 
script to be able to export the mimimal image to the host environment. By running this 'builder' docker image a new directory can be extracted 
containing a docker setup for a mimimal image. This new directory can be used to create the final docker image. 

# How to create a Celix base image

Run (in the Celix root src dir):
 - `docker build -t celix-builder -f docker/celix-builer/Dockerfile .`
 - `mkdir -p build/celix-base`
 - `cd build/celix-base`
 - `docker run celix-builder | tar x-`
 - `docker build -t celix-base .`


# How to create a docker image using Celix

Run (in the Celix root src dir):
 - `mkdir build`
 - `cd build`
 - `cmake -DENABLE_DOCKER=ON ..`
 - `make dm_example-dockerimage`
 - `docker run -t -i dm_example`
 -  Optional to build the docker images for all deployments (with the COPY instruction): `make dockerimages`


# Known Issues

 - C++ libs are missing
 - Instructions how to add your own libs (e.g zmq) are missing
 - Still need to test all deployments
