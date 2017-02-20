# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

FROM fedora:23

#Install required libs for building, docker, python and dokerize
RUN dnf group install -y "C Development Tools and Libraries"
RUN dnf -y install \
    cmake \
    ninja-build \
    make \
    git \
    java \
    libcurl-devel \
    jansson-devel \
    libffi-devel \
    libxml2-devel \
    libuuid-devel \
    python3 \
    gdb-gdbserver

#jdk installed for jar command
RUN dnf install -y java-1.8.0-openjdk-devel

#dockerize will be used to extract a single executable with required deps
RUN pip install dockerize

#Building and installing celix
ADD . /root/celix
RUN mkdir /root/celix-build
WORKDIR /root/celix-build
RUN cmake -DCMAKE_C_FLAGS="-w" ../celix
RUN make -j && make install

#Not all library (e.g. jansson / C++ (TODO)) are required by the celix framework,
#but they are still considered a requirement because many bundles will need them
ADD docker/celix-builder/libdeps /root/libdeps
RUN mkdir /root/libdeps/build
WORKDIR /root/libdeps/build
RUN cmake .. && make -j && make install

#Add Celix, gdb-server &  jansson (including softlinks)
RUN dockerize -n -o /root/docker-image --add-file /usr/lib64/libjansson.so.4 /lib64/libjansson.so.4 /usr/local/bin/celix  /usr/bin/gdbserver /usr/local/bin/dummy && rm /root/docker-image/usr/local/bin/dummy

#Optional add gdb
#RUN dockerize -n -o /root/docker-image /usr/bin/gdb

#Add ENTYPOINT instruction to Dockerfile
RUN echo "ENTRYPOINT [\"/usr/local/bin/celix\"]" >> /root/docker-image/Dockerfile

#Adding scripts
WORKDIR /root
ADD docker/celix-builder/main.sh /root/
RUN chmod +x /root/main.sh

ENTRYPOINT "/root/main.sh"
