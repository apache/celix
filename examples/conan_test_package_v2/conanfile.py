#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance
#  with the License.  You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing,
#   software distributed under the License is distributed on an
#   "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#   KIND, either express or implied.  See the License for the
#   specific language governing permissions and limitations
#   under the License.

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.build import can_run
from conan.tools.files import chdir
from conan.tools.files import copy
import os


class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "VirtualRunEnv"
    test_type = "explicit"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires(self.tested_reference_str)

    def generate(self):
        tc = CMakeToolchain(self)
        celix_options = self.dependencies["celix"].options
        tc.cache_variables["TEST_FRAMEWORK"] = celix_options.build_framework
        tc.cache_variables["TEST_HTTP_ADMIN"] = celix_options.build_http_admin
        tc.cache_variables["TEST_LOG_SERVICE"] = celix_options.build_log_service
        tc.cache_variables["TEST_SYSLOG_WRITER"] = celix_options.build_syslog_writer
        tc.cache_variables["TEST_RSA"] = celix_options.build_remote_service_admin
        tc.cache_variables["TEST_RSA_DFI"] = celix_options.build_rsa_remote_service_admin_dfi
        tc.cache_variables["TEST_RSA_SHM_V2"] = celix_options.build_rsa_remote_service_admin_shm_v2
        tc.cache_variables["TEST_RSA_RPC_JSON"] = celix_options.build_rsa_json_rpc
        tc.cache_variables["TEST_RSA_DISCOVERY_CONFIGURED"] = celix_options.build_rsa_discovery_configured
        tc.cache_variables["TEST_RSA_DISCOVERY_ETCD"] = celix_options.build_rsa_discovery_etcd
        tc.cache_variables["TEST_RSA_DISCOVERY_ZEROCONF"] = celix_options.build_rsa_discovery_zeroconf
        tc.cache_variables["TEST_SHELL"] = celix_options.build_shell
        if celix_options.build_shell:
            tc.cache_variables["TEST_CXX_SHELL"] = celix_options.celix_cxx17 or celix_options.celix_cxx14
        tc.cache_variables["TEST_REMOTE_SHELL"] = celix_options.build_remote_shell
        tc.cache_variables["TEST_SHELL_TUI"] = celix_options.build_shell_tui
        tc.cache_variables["TEST_SHELL_WUI"] = celix_options.build_shell_wui
        tc.cache_variables["TEST_ETCD_LIB"] = celix_options.build_celix_etcdlib
        tc.cache_variables["TEST_LAUNCHER"] = celix_options.build_launcher
        tc.cache_variables["TEST_PROMISES"] = celix_options.build_promises
        tc.cache_variables["TEST_PUSHSTREAMS"] = celix_options.build_pushstreams
        tc.cache_variables["TEST_LOG_HELPER"] = celix_options.build_log_helper
        tc.cache_variables["TEST_LOG_SERVICE_API"] = celix_options.build_log_service_api
        tc.cache_variables["TEST_CXX_REMOTE_SERVICE_ADMIN"] = celix_options.build_cxx_remote_service_admin
        tc.cache_variables["TEST_SHELL_API"] = celix_options.build_shell_api
        tc.cache_variables["TEST_CELIX_DFI"] = celix_options.build_celix_dfi
        tc.cache_variables["TEST_UTILS"] = celix_options.build_utils
        tc.cache_variables["TEST_COMPONENTS_READY_CHECK"] = celix_options.build_components_ready_check
        # the following is workaround https://github.com/conan-io/conan/issues/7192
        for dep in self.dependencies.host.values():
            if dep.cpp_info.libdir is not None:
                copy(self, "*", dep.cpp_info.libdir, os.path.join(self.build_folder, "lib"))
        tc.cache_variables["CMAKE_BUILD_RPATH"] = os.path.join(self.build_folder, "lib")
        tc.user_presets_path = False
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            with chdir(self, self.build_folder):
                celix_options = self.dependencies["celix"].options
                if celix_options.build_framework:
                    self.run("./conan_test_package/use_framework", env="conanrun")
                if celix_options.build_http_admin:
                    self.run("./use_http_admin", cwd=os.path.join("deploy", "use_http_admin"), env="conanrun")
                if celix_options.build_log_service:
                    self.run("./use_log_writer", cwd=os.path.join("deploy", "use_log_writer"), env="conanrun")
                if celix_options.build_syslog_writer:
                    self.run("./use_syslog_writer", cwd=os.path.join("deploy", "use_syslog_writer"), env="conanrun")
                if celix_options.build_remote_service_admin:
                    self.run("./use_my_rsa", cwd=os.path.join("deploy", "use_my_rsa"), env="conanrun")
                    self.run("./conan_test_package/use_c_rsa_spi", env="conanrun")
                if celix_options.build_rsa_remote_service_admin_dfi and celix_options.build_launcher:
                    self.run("./use_rsa_dfi", cwd=os.path.join("deploy", "use_rsa_dfi"), env="conanrun")
                if celix_options.build_rsa_remote_service_admin_shm_v2:
                    self.run("./use_rsa_shm_v2", cwd=os.path.join("deploy", "use_rsa_shm_v2"), env="conanrun")
                if celix_options.build_rsa_json_rpc:
                    self.run("./use_rsa_rpc_json", cwd=os.path.join("deploy", "use_rsa_rpc_json"), env="conanrun")
                if celix_options.build_rsa_discovery_configured and celix_options.build_launcher:
                    self.run("./use_rsa_configured", cwd=os.path.join("deploy", "use_rsa_configured"), env="conanrun")
                if celix_options.build_rsa_discovery_etcd and celix_options.build_launcher:
                    self.run("./use_rsa_etcd", cwd=os.path.join("deploy", "use_rsa_etcd"), env="conanrun")
                if celix_options.build_rsa_discovery_zeroconf:
                    self.run("./use_rsa_discovery_zeroconf",
                             cwd=os.path.join("deploy", "use_rsa_discovery_zeroconf"), env="conanrun")
                if celix_options.build_shell:
                    self.run("./conan_test_package/use_shell", env="conanrun")
                    if celix_options.celix_cxx17 or celix_options.celix_cxx14:
                        self.run("./conan_test_package/use_cxx_shell", env="conanrun")
                if celix_options.build_remote_shell:
                    self.run("./use_remote_shell", cwd=os.path.join("deploy", "use_remote_shell"), env="conanrun")
                if celix_options.build_shell_tui:
                    self.run("./use_shell_tui", cwd=os.path.join("deploy", "use_shell_tui"), env="conanrun")
                if celix_options.build_shell_wui:
                    self.run("./use_shell_wui", cwd=os.path.join("deploy", "use_shell_wui"), env="conanrun")
                if celix_options.build_celix_etcdlib:
                    self.run("./conan_test_package/use_etcd_lib", env="conanrun")
                if celix_options.build_launcher:
                    self.run("./use_launcher", cwd=os.path.join("deploy", "use_launcher"), env="conanrun")
                if celix_options.build_promises:
                    self.run("./conan_test_package/use_promises", env="conanrun")
                if celix_options.build_pushstreams:
                    self.run("./conan_test_package/use_pushstreams", env="conanrun")
                if celix_options.build_log_helper:
                    self.run("./conan_test_package/use_log_helper", env="conanrun")
                if celix_options.build_log_service_api:
                    self.run("./conan_test_package/use_log_service_api", env="conanrun")
                if celix_options.build_cxx_remote_service_admin:
                    self.run("./use_cxx_remote_service_admin",
                             cwd=os.path.join("deploy", "use_cxx_remote_service_admin"), env="conanrun")
                    self.run("./conan_test_package/use_rsa_spi", env="conanrun")
                if celix_options.build_shell_api:
                    self.run("./conan_test_package/use_shell_api", env="conanrun")
                if celix_options.build_celix_dfi:
                    self.run("./conan_test_package/use_celix_dfi", env="conanrun")
                if celix_options.build_utils:
                    self.run("./conan_test_package/use_utils", env="conanrun")
                if celix_options.build_components_ready_check:
                    self.run("./use_components_ready_check",
                             cwd=os.path.join("deploy", "use_components_ready_check"), env="conanrun")
