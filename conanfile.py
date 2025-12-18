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

from conan import ConanFile, conan_version
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain
from conan.tools.scm import Version
from conan.tools.files import copy
import os


required_conan_version = ">=1.32.0"


class CelixConan(ConanFile):
    name = "celix"
    version = "3.0.0"
    homepage = "https://celix.apache.org"
    url = "https://github.com/apache/celix.git"
    topics = ("conan", "celix", "osgi", "embedded", "linux", "C/C++")
    exports_sources = ("CMakeLists.txt", "bundles*", "cmake*", "!cmake-build*", "examples*", "libs*", "misc*",
                       "LICENSE", "!examples/conan_test_package*")
    generators = "CMakeDeps", "VirtualRunEnv"
    settings = "os", "arch", "compiler", "build_type"
    license = " Apache-2.0"
    description = "Apache Celix is an implementation of the OSGi specification adapted to C and C++ (C++14). " \
                  "It is a framework to develop (dynamic) modular software applications " \
                  "using component and/or service-oriented programming."

    _celix_defaults = {
        "enable_testing": False,
        "enable_code_coverage": False,
        "enable_address_sanitizer": False,
        "enable_undefined_sanitizer": False,
        "enable_thread_sanitizer": False,
        "enable_fuzzing": False,
        "enable_benchmarking": False,
        "install_find_modules": False,
        "build_all": False,
        "build_http_admin": False,
        "build_log_service": False,
        "build_log_helper": False,
        "build_log_service_api": False,
        "build_syslog_writer": False,
        "build_cxx_remote_service_admin": False,
        "build_cxx_rsa_integration": False,
        "build_remote_service_admin": False,
        "build_rsa_remote_service_admin_dfi": False,
        "build_rsa_discovery_common": False,
        "build_rsa_discovery_configured": False,
        "build_rsa_discovery_etcd": False,
        "build_rsa_remote_service_admin_shm_v2": False,
        "build_rsa_json_rpc": False,
        "build_rsa_discovery_zeroconf": False,
        "build_shell": False,
        "build_shell_api": False,
        "build_remote_shell": False,
        "build_shell_tui": False,
        "build_shell_wui": False,
        "build_components_ready_check": False,
        "build_examples": False,
        "build_celix_etcdlib":  False,
        "build_launcher": False,
        "build_promises": False,
        "build_pushstreams": False,
        "build_experimental": False,
        "build_celix_dfi": False,
        "build_framework": False,
        "build_rcm": False,
        "build_utils": False,
        "build_event_admin": False,
        "build_event_admin_examples": False,
        "build_event_admin_remote_provider_mqtt": False,
        "celix_cxx14": True,
        "celix_cxx17": True,
        "celix_install_deprecated_api": False,
        "celix_use_compression_for_bundle_zips": True,
        "enable_cmake_warning_tests": False,
        "enable_testing_on_ci": False,
        "framework_curlinit": True,
        "enable_ccache": False,
        "enable_deprecated_warnings": False,
    }
    options = {
        "celix_err_buffer_size": ["ANY"],
        "celix_utils_max_strlen": ["ANY"],
        "celix_properties_optimization_string_buffer_size": ["ANY"],
        "celix_properties_optimization_entries_buffer_size": ["ANY"],
    }
    default_options = {
        "celix_err_buffer_size": "512",
        "celix_utils_max_strlen": "1073741824",
        "celix_properties_optimization_string_buffer_size": "128",
        "celix_properties_optimization_entries_buffer_size": "16",
    }

    for comp in _celix_defaults.keys():
        options[comp] = [True, False]
    del comp

    _cmake = None

    def validate_config_option_is_positive_number(self, option):
        try:
            val = int(self.options.get_safe(option))
            if val <= 0:
                raise ValueError
        except ValueError:
            raise ConanInvalidConfiguration("{} must be a positive number".format(option))

    def validate(self):
        if self.settings.os != "Linux" and self.settings.os != "Macos":
            raise ConanInvalidConfiguration("Celix is only supported for Linux/Macos")

        if self.options.build_rsa_remote_service_admin_shm_v2 and self.settings.os != "Linux":
            raise ConanInvalidConfiguration("Celix build_rsa_remote_service_admin_shm_v2 is only supported for Linux")

        if self.options.build_rsa_discovery_zeroconf and self.settings.os != "Linux":
            raise ConanInvalidConfiguration("Celix build_rsa_discovery_zeroconf is only supported for Linux")

        if self.options.enable_fuzzing and self.settings.compiler != "clang" and self.settings.compiler != "apple-clang":
            raise ConanInvalidConfiguration("Celix enable_fuzzing=True requires the 'clang' compiler")

        self.validate_config_option_is_positive_number("celix_err_buffer_size")
        self.validate_config_option_is_positive_number("celix_utils_max_strlen")
        self.validate_config_option_is_positive_number("celix_properties_optimization_string_buffer_size")
        self.validate_config_option_is_positive_number("celix_properties_optimization_entries_buffer_size")

    def package_id(self):
        del self.info.options.build_all
        # the followings are not installed
        del self.info.options.build_cxx_rsa_integration
        del self.info.options.build_examples
        del self.info.options.build_event_admin_examples
        del self.info.options.enable_cmake_warning_tests
        del self.info.options.enable_testing_on_ci
        del self.info.options.enable_ccache
        del self.info.options.enable_deprecated_warnings
        del self.info.options.enable_testing
        del self.info.options.enable_benchmarking
        del self.info.options.enable_fuzzing
        del self.info.options.enable_code_coverage

    def build_requirements(self):
        if self.options.enable_testing:
            self.test_requires("gtest/1.10.0")
        if self.options.enable_ccache:
            self.build_requires("ccache/4.7.4")
        if self.options.enable_benchmarking:
            self.test_requires("benchmark/[>=1.6.2]")

    def configure(self):
        # copy options to options, fill in defaults if not set
        options = {}
        for opt in self._celix_defaults.keys():
            options[opt] = self.options.get_safe(opt).value
            if options[opt] is None:
                options[opt] = self._celix_defaults[opt]

        if options["build_all"]:
            for opt in options.keys():
                if opt.startswith('build_'):
                    options[opt] = True

        if options["build_event_admin_examples"]:
            options["build_event_admin"] = True
            options["build_log_service"] = True
            options["build_shell_tui"] = True
            options["build_launcher"] = True
            options["build_event_admin_remote_provider_mqtt"] = True
            options["build_rsa_discovery_zeroconf"] = True

        if self.settings.os != "Linux":
            options["build_rsa_remote_service_admin_shm_v2"] = False
            options["build_rsa_discovery_zeroconf"] = False

        if options["enable_code_coverage"]:
            options["enable_testing"] = True

        if options["build_examples"]:
            options["build_shell_tui"] = True
            options["build_shell_wui"] = True
            options["build_log_service"] = True
            options["build_syslog_writer"] = True

        if options["build_event_admin_remote_provider_mqtt"]:
            options["build_event_admin"] = True
            options["build_remote_service_admin"] = True
            options["build_shell_api"] = True

        if options["build_cxx_rsa_integration"]:
            options["build_cxx_remote_service_admin"] = True
            options["build_pushstreams"] = True
            options["build_promises"] = True
            options["build_log_helper"] = True
            options["build_shell"] = True
            options["build_shell_tui"] = True
            options["build_shell_api"] = True

        if options["build_cxx_remote_service_admin"]:
            options["build_framework"] = True
            options["build_log_helper"] = True
            options["celix_cxx17"] = True

        if options["build_rsa_discovery_etcd"]:
            options["build_celix_etcdlib"] = True
            options["build_rsa_discovery_common"] = True

        if options["build_rsa_discovery_configured"]:
            options["build_rsa_discovery_common"] = True

        if options["build_rsa_discovery_common"] or options["build_rsa_discovery_zeroconf"] \
                or options["build_rsa_remote_service_admin_dfi"] or options["build_rsa_json_rpc"] \
                or options["build_rsa_remote_service_admin_shm_v2"]:
            options["build_remote_service_admin"] = True

        if options["build_remote_service_admin"]:
            options["build_framework"] = True
            options["build_log_helper"] = True
            options["build_celix_dfi"] = True
            options["celix_install_deprecated_api"] = True

        if options["build_event_admin"]:
            options["build_framework"] = True
            options["build_log_helper"] = True

        if options["build_remote_shell"]:
            options["build_shell"] = True

        if options["build_shell_wui"]:
            options["build_shell"] = True
            options["build_http_admin"] = True

        if options["build_shell_tui"]:
            options["build_shell"] = True

        if options["build_shell"]:
            options["build_shell_api"] = True
            options["build_log_helper"] = True
            options["build_framework"] = True

        if options["build_http_admin"]:
            options["build_framework"] = True

        if options["build_syslog_writer"]:
            options["build_log_service"] = True

        if options["build_log_service"]:
            options["build_log_service_api"] = True
            options["build_shell_api"] = True
            options["build_framework"] = True
            options["build_log_helper"] = True

        if options["build_shell_api"]:
            options["build_utils"] = True

        if options["build_log_helper"]:
            options["build_log_service_api"] = True
            options["build_framework"] = True

        if options["build_log_service_api"]:
            options["build_utils"] = True
            if options["celix_install_deprecated_api"]:
                options["build_framework"] = True

        if options["build_components_ready_check"]:
            options["build_framework"] = True

        if options["build_rcm"]:
            options["build_utils"] = True

        if options["build_launcher"]:
            options["build_framework"] = True

        if options["build_celix_dfi"]:
            options["build_utils"] = True

        if options["build_framework"]:
            options["build_utils"] = True

        if options["build_pushstreams"]:
            options["build_promises"] = True

        if options["build_promises"]:
            options["celix_cxx17"] = True

        if options["celix_cxx17"]:
            options["celix_cxx14"] = True

        for opt in self._celix_defaults.keys():
            setattr(self.options, opt, options[opt])
        del options

        # Conan 2 does not support set dependency option in requirements()
        # https://github.com/conan-io/conan/issues/14528#issuecomment-1685344080
        if self.options.build_utils:
            self.options['libzip'].shared = True
        if self.options.build_framework:
            self.options['util-linux-libuuid'].shared = True
        if ((self.options.build_framework and self.options.framework_curlinit)
                or self.options.build_celix_etcdlib
                or self.options.build_rsa_discovery_common or self.options.build_rsa_remote_service_admin_dfi
                or self.options.build_launcher):
            self.options['libcurl'].shared = True
            self.options['openssl'].shared = True
        if self.options.enable_testing:
            self.options['gtest'].shared = True
        if self.options.enable_benchmarking:
            self.options['benchmark'].shared = True
        if (self.options.build_rsa_discovery_common
                or (self.options.build_rsa_remote_service_admin_dfi and self.options.enable_testing)):
            self.options['libxml2'].shared = True
        if self.options.build_http_admin or self.options.build_rsa_discovery_common \
                or self.options.build_rsa_remote_service_admin_dfi:
            self.options['civetweb'].shared = True
            self.options['openssl'].shared = True
        if self.options.build_celix_dfi:
            self.options['libffi'].shared = True
        if self.options.build_utils or self.options.build_celix_dfi or self.options.build_celix_etcdlib or self.options.build_event_admin_remote_provider_mqtt:
            self.options['jansson'].shared = True
        if self.options.build_event_admin_remote_provider_mqtt:
            self.options['mosquitto'].shared = True
            if self.options.enable_testing:
                self.options['mosquitto'].broker = True

    def requirements(self):
        if self.options.build_utils:
            self.requires("libzip/[>=1.7.3 <2.0.0]")
        if self.options.build_framework:
            self.requires("util-linux-libuuid/[>=2.39 <3.0.0]")
            if self.settings.os == "Macos":
                self.requires("gettext/0.21") #needed on MacOS 15 by libuuid
        if ((self.options.build_framework and self.options.framework_curlinit)
                or self.options.build_celix_etcdlib
                or self.options.build_rsa_discovery_common or self.options.build_rsa_remote_service_admin_dfi
                or self.options.build_launcher):
            self.requires("libcurl/[>=8.4.0 <9.0.0]")
        if (self.options.build_rsa_discovery_common
                or (self.options.build_rsa_remote_service_admin_dfi and self.options.enable_testing)):
            self.requires("libxml2/[>=2.9.9 <3.0.0]")
        if self.options.build_cxx_remote_service_admin:
            self.requires("rapidjson/[>=1.1.0 <2.0.0]")
        if self.options.build_http_admin or self.options.build_rsa_discovery_common \
                or self.options.build_rsa_remote_service_admin_dfi:
            self.requires("civetweb/1.16")
        if self.options.build_celix_dfi:
            self.requires("libffi/[>=3.2.1 <4.0.0]")
        if self.options.build_utils or self.options.build_celix_dfi or self.options.build_celix_etcdlib or self.options.build_event_admin_remote_provider_mqtt:
            self.requires("jansson/[>=2.12 <3.0.0]")
        if self.options.build_rsa_discovery_zeroconf:
            # TODO: To be replaced with mdnsresponder/1790.80.10, resolve some problems of mdnsresponder
            # https://github.com/conan-io/conan-center-index/pull/16254
            self.requires("mdnsresponder/1310.140.1")
        self.requires("openssl/[>=3.2.0]", override=True)
        # Fix zlib to 1.3.1, 'libzip/1.10.1' and 'libcurl/7.64.1' requires different zlib versions causing conflicts
        self.requires("zlib/1.3.1", override=True)
        if self.options.build_event_admin_remote_provider_mqtt:
            self.requires("mosquitto/[>=2.0.3 <3.0.0]")
        self.validate()

    def generate(self):
        tc = CMakeToolchain(self)
        for opt in self._celix_defaults.keys():
            tc.cache_variables[opt.upper()] = self.options.get_safe(opt)
        if self.options.enable_testing:
            lst = [x.ref.name for x in self.requires.values()]
            if "mdnsresponder" in lst:
                tc.cache_variables["BUILD_ERROR_INJECTOR_MDNSRESPONDER"] = "ON"
            if "jansson" in lst:
                tc.cache_variables["BUILD_ERROR_INJECTOR_JANSSON"] = "ON"
            if "mosquitto" in lst:
                tc.cache_variables["BUILD_ERROR_INJECTOR_MOSQUITTO"] = "ON"
            if "libcurl" in lst:
                tc.cache_variables["BUILD_ERROR_INJECTOR_CURL"] = "ON"
        tc.cache_variables["CELIX_ERR_BUFFER_SIZE"] = str(self.options.celix_err_buffer_size)
        # tc.cache_variables["CMAKE_PROJECT_Celix_INCLUDE"] = os.path.join(self.build_folder, "conan_paths.cmake")
        # the following is workaround for https://github.com/conan-io/conan/issues/7192
        if self.settings.os == "Linux":
            tc.cache_variables["CMAKE_EXE_LINKER_FLAGS"] = "-Wl,--unresolved-symbols=ignore-in-shared-libs"
        v = Version(self.version)
        tc.cache_variables["CELIX_MAJOR"] = str(v.major.value)
        tc.cache_variables["CELIX_MINOR"] = str(v.minor.value)
        tc.cache_variables["CELIX_MICRO"] = str(v.patch.value)
        tc.cache_variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = "ON"
        tc.generate()

    def _configure_cmake(self):
        if self._cmake:
            return self._cmake
        self._cmake = CMake(self)
        self._cmake.configure()
        return self._cmake

    def build(self):
        # self._patch_sources()
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        # enable imports() of conanfile.py to collect bundles from the local cache using @bindirs
        # check https://docs.conan.io/en/latest/reference/conanfile/methods.html#imports
        self.cpp_info.bindirs = ["bin", os.path.join("share", self.name, "bundles")]
        self.cpp_info.build_modules["cmake"].append(os.path.join("lib", "cmake", "Celix", "CelixConfig.cmake"))
        self.cpp_info.build_modules["cmake_find_package"].append(os.path.join("lib", "cmake",
                                                                              "Celix", "CelixConfig.cmake"))
        self.cpp_info.set_property("cmake_build_modules", [os.path.join("lib", "cmake", "Celix", "CelixConfig.cmake")])
