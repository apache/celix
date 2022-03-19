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

from conans import ConanFile, CMake, tools
from conans.errors import ConanInvalidConfiguration
import os


required_conan_version = ">=1.32.0"


class CelixConan(ConanFile):
    name = "celix"
    homepage = "https://celix.apache.org"
    url = "https://github.com/apache/celix.git"
    topics = ("conan", "celix", "osgi", "embedded", "linux", "C/C++")
    exports_sources = "CMakeLists.txt", "bundles*", "cmake*", "!cmake-build*", "examples*", "libs*", "misc*", "LICENSE"
    generators = "cmake_paths", "cmake_find_package"
    settings = "os", "arch", "compiler", "build_type"
    license = " Apache-2.0"
    description = "Apache Celix is an implementation of the OSGi specification adapted to C and C++ (C++17). " \
                  "It is a framework to develop (dynamic) modular software applications " \
                  "using component and/or service-oriented programming."

    tool_requires = "cmake/3.17.5"

    options = {
        "enable_testing": [True, False],
        "celix_add_openssl_dep": [True, False],
        "build_all": [True, False],
        "build_deployment_admin": [True, False],
        "build_device_access_example": [True, False],
        "build_device_access": [True, False],
        "build_http_admin": [True, False],
        "build_log_service": [True, False],
        "build_syslog_writer": [True, False],
        "build_pubsub": [True, False],
        "build_pubsub_psa_zmq": [True, False],
        "build_pubsub_examples": [True, False],
        "build_pubsub_integration": [True, False],
        "build_pubsub_psa_tcp": [True, False],
        "build_pubsub_psa_udp_mc": [True, False],
        "build_pubsub_psa_ws": [True, False],
        "build_pubsub_discovery_etcd": [True, False],
        "build_cxx_remote_service_admin": [True, False],
        "build_cxx_rsa_integration": [True, False],
        "build_remote_service_admin": [True, False],
        "build_rsa_remote_service_admin_dfi": [True, False],
        "build_rsa_discovery_configured": [True, False],
        "build_rsa_discovery_etcd": [True, False],
        "build_shell": [True, False],
        "build_remote_shell": [True, False],
        "build_shell_bonjour": [True, False],
        "build_shell_tui": [True, False],
        "build_shell_wui": [True, False],
        "build_examples": [True, False],
        "build_celix_etcdlib": [True, False],
        "build_launcher": [True, False],
        "build_promises": [True, False],
        "build_pushstreams": [True, False],
        "celix_cxx": [True, False],
        "celix_install_deprecated_api": [True, False],
        "celix_add_deprecated_attributes": [True, False],
    }
    default_options = { 
        "enable_testing": False,
        "celix_add_openssl_dep": False,
        "build_all": False,
        "build_deployment_admin": False,
        "build_device_access": False,
        "build_device_access_example": False,
        "build_http_admin": True,
        "build_log_service": True,
        "build_syslog_writer": True,
        "build_pubsub": True,
        "build_pubsub_psa_zmq": False,
        "build_pubsub_examples": False,
        "build_pubsub_integration": False,
        "build_pubsub_psa_tcp": False,
        "build_pubsub_psa_udp_mc": False,
        "build_pubsub_psa_ws": True,
        "build_pubsub_discovery_etcd": False,
        "build_cxx_remote_service_admin": False,
        "build_cxx_rsa_integration": False,
        "build_remote_service_admin": True,
        "build_rsa_remote_service_admin_dfi": True,
        "build_rsa_discovery_configured": True,
        "build_rsa_discovery_etcd": False,
        "build_shell": True,
        "build_remote_shell": False,
        "build_shell_bonjour": False,
        "build_shell_tui": False,
        "build_shell_wui": False,
        "build_examples": False,
        "build_celix_etcdlib":  False,
        "build_launcher": False,
        "build_promises": False,
        "build_pushstreams": False,
        "celix_cxx": False,
        "celix_install_deprecated_api": False,
        "celix_add_deprecated_attributes": True,
    }
    _cmake = None

    def validate(self):
        if self.settings.os != "Linux" and self.settings.os != "Macos":
            raise ConanInvalidConfiguration("Celix is only supported for Linux/Macos")

    def package_id(self):
        del self.info.options.build_all
        del self.info.options.enable_testing
        del self.info.options.celix_add_openssl_dep
        # the followings are not installed
        del self.info.options.build_device_access_example
        del self.info.options.build_pubsub_integration
        del self.info.options.build_pubsub_examples
        del self.info.options.build_cxx_rsa_integration
        del self.info.options.build_examples
        del self.info.options.build_shell_bonjour

    def build_requirements(self):
        if self.options.enable_testing:
            self.test_requires("gtest/1.10.0")
            self.test_requires("cpputest/4.0")

    def configure(self):
        if self.options.build_all:
            for opt, val in self.options.values.items():
                if opt.startswith('build_'):
                    setattr(self.options, opt, True)
        if not self.options.celix_cxx:
            self.options.build_cxx_remote_service_admin = False
            self.options.build_promises = False
            self.options.build_pushstreams = False
        if not self.options.build_cxx_remote_service_admin:
            self.options.build_cxx_rsa_integration = False
        if not self.options.enable_testing:
            self.options.build_pubsub_integration = False
        if not self.options.build_device_access:
            self.options.build_device_access_example = False
        if not self.options.build_log_service:
            self.options.build_syslog_writer = False
        if not self.options.build_pubsub:
            self.options.build_pubsub_psa_zmq = False
            self.options.build_pubsub_examples = False
            self.options.build_pubsub_integration = False
            self.options.build_pubsub_psa_tcp = False
            self.options.build_pubsub_psa_udp_mc = False
            self.options.build_pubsub_psa_ws = False
            self.options.build_pubsub_discovery_etcd = False
        if not self.options.build_remote_service_admin:
            self.options.build_rsa_remote_service_admin_dfi = False
            self.options.build_rsa_discovery_configured = False
            self.options.build_rsa_discovery_etcd = False
        if not self.options.build_shell:
            self.options.build_remote_shell = False
            self.options.build_shell_bonjour = False
            self.options.build_shell_tui = False
            self.options.build_shell_wui = False
        if not self.options.celix_install_deprecated_api:
            self.options.build_shell_bonjour = False

    def requirements(self):
        # libffi/3.3@zhengpeng/testing is a workaround of the following buggy commit:
        # https://github.com/conan-io/conan-center-index/pull/5085#issuecomment-847487808
        #self.requires("libffi/3.3@zhengpeng/testing")
        self.requires("libffi/[~3.2.1]")
        self.options['libffi'].shared = True
        self.requires("jansson/[~2.12]")
        self.options['jansson'].shared = True
        self.requires("libcurl/[~7.64.1]")
        self.options['libcurl'].shared = True
        self.requires("zlib/[~1.2.8]")
        self.options['zlib'].shared = True
        self.requires("libuuid/1.0.3")
        self.options['libuuid'].shared = True
        self.requires("libzip/[~1.7.3]")
        self.options['libzip'].shared = True
        self.options['openssl'].shared = True
        self.options['libxml2'].shared = True
        if self.options.enable_testing:
            self.options['gtest'].shared = True
        if self.options.celix_add_openssl_dep:
            self.requires("openssl/1.1.1k")
        if self.options.build_remote_service_admin or self.options.build_shell_bonjour:
            self.requires("libxml2/[~2.9.9]")
        if self.options.build_shell_bonjour:
            # TODO: CC=cc is fixed in the official mdnsresponder Makefile, patching is needed to make cross-compile work
            # https://github.com/conan-io/conan-center-index/issues/9711
            # Another issue is in conan infrastructure: https://github.com/conan-io/conan-center-index/issues/9709
            self.requires("mdnsresponder/1310.140.1")
        if self.options.build_pubsub_psa_zmq:
            self.requires("zeromq/4.3.2")
            self.options['zeromq'].shared = True
            self.requires("czmq/4.2.0")
            self.options['czmq'].shared = True

    def _configure_cmake(self):
        if self._cmake:
            return self._cmake
        self._cmake = CMake(self)
        for opt, val in self.options.values.items():
            self._cmake.definitions[opt.upper()] = self.options.get_safe(opt, False)
        self._cmake.definitions["CMAKE_PROJECT_Celix_INCLUDE"] = os.path.join(self.build_folder, "conan_paths.cmake")
        # the followint is workaround https://github.com/conan-io/conan/issues/7192
        self._cmake.definitions["CMAKE_EXE_LINKER_FLAGS"] = "-Wl,--unresolved-symbols=ignore-in-shared-libs"
        self.output.info(self._cmake.definitions)
        v = tools.Version(self.version)
        self._cmake.configure(defs={'CELIX_MAJOR': v.major, 'CELIX_MINOR': v.minor, 'CELIX_MICRO': v.patch})
        return self._cmake

    def build(self):
        # self._patch_sources()
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy("LICENSE", dst="licenses", src=self.source_folder)
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.bindirs = ["bin", os.path.join("share", self.name, "bundles")]
        self.cpp_info.build_modules["cmake"].append(os.path.join("lib", "cmake", "Celix", "CelixConfig.cmake"))
