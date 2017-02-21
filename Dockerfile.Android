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

# Celix android builder
# 
# Howto:
#
# Build docker image -> docker build -t celix-android-builder - < Dockerfile.Android
# Run docker image -> docker run --name builder celix-android-builder
# Extract filesystem -> docker export builder > fs.tar
# Extract /build dir from tar -> tar xf fs.tar build/output/celix
#
#

FROM ubuntu:14.04
MAINTAINER Bjoern Petri <bjoern.petri@sundevil.de>

ENV ARCH armv7
ENV PLATFORM android-18
ENV ANDROID_STANDALONE_TOOLCHAIN  /build/toolchain-arm
ENV ANDROID_CMAKE_HOME /build/resources/android-cmake
ENV SYSROOT $ANDROID_STANDALONE_TOOLCHAIN/sysroot
ENV PATH $ANDROID_STANDALONE_TOOLCHAIN/bin:$ANDROID_STANDALONE_TOOLCHAIN/usr/local/bin:$PATH
ENV PATH $PATH:/build/resources/android-ndk-r10e
ENV CROSS_COMPILE arm-linux-androideabi
ENV CC arm-linux-androideabi-gcc
ENV CCX arm-linux-androideabi-g++
ENV AR arm-linux-androideabi-ar
ENV AS arm-linux-androideabi-as
ENV LD arm-linux-androideabi-ld
ENV RANLIB arm-linux-androideabi-ranlib
ENV NM arm-linux-androideabi-nm
ENV STRIP arm-linux-androideabi-strip
ENV CHOST arm-linux-androideabi

# install needed tools

RUN apt-get update && apt-get install -y software-properties-common && add-apt-repository ppa:george-edison55/cmake-3.x && apt-get clean

RUN apt-get update && apt-get install -y \
    automake \
    build-essential \
    wget \
    p7zip-full \
    bash \
    curl \
    cmake \
    git \
    vim \
    zip \
    && apt-get clean


RUN mkdir -p build/output

WORKDIR /build/resources


# Extracting ndk/sdk
# create standalone toolchain
RUN curl -L -O http://dl.google.com/android/ndk/android-ndk-r10e-linux-x86_64.bin && \
	chmod a+x android-ndk-r10e-linux-x86_64.bin && \
	7z x android-ndk-r10e-linux-x86_64.bin  > /dev/null && \
	bash android-ndk-r10e/build/tools/make-standalone-toolchain.sh --verbose --platform=$PLATFORM --install-dir=$ANDROID_STANDALONE_TOOLCHAIN --arch=arm --toolchain=arm-linux-androideabi-4.9 --system=linux-x86_64



# uuid
RUN curl -L -O http://downloads.sourceforge.net/libuuid/libuuid-1.0.3.tar.gz && \
	tar -xvzf libuuid-1.0.3.tar.gz && \
	cd libuuid-1.0.3 && \
	./configure --host=arm-linux-androideabi  --disable-shared --enable-static --prefix=/build/output/uuid && \
	make && make install


# zlib
RUN curl -L -O http://zlib.net/zlib-1.2.11.tar.gz && \
	tar -xvzf zlib-*.tar.gz && \
	cd zlib-* && \
	./configure --prefix=/build/output/zlib && make && make install

# curl
RUN curl -L -O http://curl.haxx.se/download/curl-7.38.0.tar.gz && \
	tar -xvzf curl-7.38.0.tar.gz && \ 
	cd curl-7.38.0 && \
	./configure --host=arm-linux-androideabi --disable-shared --enable-static --disable-dependency-tracking --with-zlib=`pwd`/../../output/zlib --without-ca-bundle --without-ca-path --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-proxy --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smtp --disable-gopher --disable-sspi --disable-manual --target=arm-linux-androideabi --build=x86_64-unknown-linux-gnu --prefix=/build/output/curl && \
	make && make install

# jansson
RUN curl -L -O http://www.digip.org/jansson/releases/jansson-2.7.tar.bz2 && \
	tar -xvjf jansson-2.7.tar.bz2 && \
	cd jansson-2.7 && ./configure --host=arm-linux-androideabi  --disable-shared --enable-static --prefix=/build/output/jansson && \
	make && make install


# libmxl2
RUN curl -L -O http://xmlsoft.org/sources/libxml2-2.7.2.tar.gz && \
	curl -L -O https://raw.githubusercontent.com/bpetri/libxml2_android/master/config.guess && \
	curl -L -O https://raw.githubusercontent.com/bpetri/libxml2_android/master/config.sub && \
	curl -L -O https://raw.githubusercontent.com/bpetri/libxml2_android/master/libxml2.patch && \ 
	tar -xvzf libxml2-2.7.2.tar.gz && cp config.guess config.sub libxml2-2.7.2 && \
	patch -p1 < libxml2.patch && \
	cd libxml2-2.7.2 && \
	./configure --host=arm-linux-androideabi  --disable-shared --enable-static --prefix=/build/output/libxml2 && \
	make && make install


# libffi
RUN curl -L -O ftp://sourceware.org/pub/libffi/libffi-3.2.1.tar.gz && \
	tar -xvzf libffi-3.2.1.tar.gz  && \ 
	cd libffi-3.2.1 && ./configure --host=arm-linux-androideabi  --disable-shared --enable-static --prefix=/build/output/libffi && \
  	echo "#define FFI_MMAP_EXEC_WRIT 1" >> ./arm-unknown-linux-androideabi/fficonfig.h && \
  	echo "#define FFI_MMAP_EXEC_SELINUX 0" >> ./arm-unknown-linux-androideabi/fficonfig.h && \
	make && make install
	

# finally add celix src

RUN git clone -b develop --single-branch https://github.com/apache/celix.git celix

CMD mkdir -p celix/build-android && cd celix/build-android && cmake -DANDROID=TRUE -DENABLE_TESTING=OFF -DBUILD_EXAMPLES=OFF -DBUILD_DEPENDENCY_MANAGER_CXX=OFF -DBUILD_REMOTE_SERVICE_ADMIN=ON -DBUILD_REMOTE_SHELL=ON -DBUILD_RSA_DISCOVERY_CONFIGURED=ON -DBUILD_RSA_DISCOVERY_ETCD=ON -DBUILD_RSA_EXAMPLES=ON -DBUILD_RSA_REMOTE_SERVICE_ADMIN_HTTP=ON -DBUILD_RSA_REMOTE_SERVICE_ADMIN_DFI=OFF -DBUILD_RSA_TOPOLOGY_MANAGER=ON -DFFI_LIBRARY=/build/output/libffi/lib/libffi.a -DFFI_INCLUDE_DIR=/build/output/libffi/lib/libffi-3.2.1/include -DJANSSON_LIBRARY=/build/output/jansson/lib/libjansson.a -DJANSSON_INCLUDE_DIR=/build/output/jansson/include -DCURL_LIBRARY=/build/output/curl/lib/libcurl.a -DCURL_INCLUDE_DIR=/build/output/curl/include -DLIBXML2_LIBRARIES=/build/output/libxml2/lib/libxml2.a -DLIBXML2_INCLUDE_DIR=/build/output/libxml2/include/libxml2 -DZLIB_LIBRARY=/build/output/zlib/lib/libz.a -DZLIB_INCLUDE_DIR=/build/output/zlib/include -DUUID_LIBRARY=/build/output/uuid/lib/libuuid.a -DUUID_INCLUDE_DIR=/build/output/uuid/include -DCMAKE_INSTALL_PREFIX:PATH=/build/output/celix .. && make && make install

# done
