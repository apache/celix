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

FROM docker.io/library/ubuntu:22.04@sha256:b492494d8e0113c4ad3fe4528a4b5ff89faa5331f7d52c5c138196f69ce176a6

# Install dependencies
RUN DEBIAN_FRONTEND="noninteractive" apt-get update && \
    DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
        # Development dependencies \
        dos2unix \
        git \
        locales-all \
        rsync \
        tar \
        ssh \
        tzdata \
        sudo \
        vim \
        # Build dependencies \
        build-essential \
        ccache \
        cmake \
        cmake-curses-gui \
        curl \
        default-jdk \
        gcc \
        g++ \
        gdb \
        make \
        lcov \
        python3 \
        python3-pip \
        # Celix dependencies \
        civetweb \
        libavahi-compat-libdnssd-dev \
        libcivetweb-dev \
        libcpputest-dev \
        libcurl4-openssl-dev \
        libczmq-dev \
        libffi-dev \
        libjansson-dev \
        libxml2-dev \
        libzip-dev \
        rapidjson-dev \
        uuid-dev && \
    apt-get clean

# Install conan
RUN pip3 install conan && pip3 cache purge

# Add user celixdev with sudo rights and a "celixdev" password
RUN useradd -m -s /bin/bash -G sudo celixdev
RUN echo "celixdev:celixdev" | chpasswd
USER celixdev
