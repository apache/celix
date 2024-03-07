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
from conan.tools.files import copy
from conans import tools
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
        tc.cache_variables["TEST_FRAMEWORK"] = self.options["celix"].build_framework
        tc.cache_variables["TEST_HTTP_ADMIN"] = self.options["celix"].build_http_admin
        tc.cache_variables["TEST_LOG_SERVICE"] = self.options["celix"].build_log_service
        tc.cache_variables["TEST_SYSLOG_WRITER"] = self.options["celix"].build_syslog_writer
        tc.cache_variables["TEST_RSA"] = self.options["celix"].build_remote_service_admin
        tc.cache_variables["TEST_RSA_DFI"] = self.options["celix"].build_rsa_remote_service_admin_dfi
        tc.cache_variables["TEST_RSA_SHM_V2"] = self.options["celix"].build_rsa_remote_service_admin_shm_v2
        tc.cache_variables["TEST_RSA_RPC_JSON"] = self.options["celix"].build_rsa_json_rpc
        tc.cache_variables["TEST_RSA_DISCOVERY_CONFIGURED"] = self.options["celix"].build_rsa_discovery_configured
        tc.cache_variables["TEST_RSA_DISCOVERY_ETCD"] = self.options["celix"].build_rsa_discovery_etcd
        tc.cache_variables["TEST_RSA_DISCOVERY_ZEROCONF"] = self.options["celix"].build_rsa_discovery_zeroconf
        tc.cache_variables["TEST_SHELL"] = self.options["celix"].build_shell
        if self.options["celix"].build_shell:
            tc.cache_variables["TEST_CXX_SHELL"] = self.options["celix"].celix_cxx17 or self.options["celix"].celix_cxx14
        tc.cache_variables["TEST_REMOTE_SHELL"] = self.options["celix"].build_remote_shell
        tc.cache_variables["TEST_SHELL_TUI"] = self.options["celix"].build_shell_tui
        tc.cache_variables["TEST_SHELL_WUI"] = self.options["celix"].build_shell_wui
        tc.cache_variables["TEST_ETCD_LIB"] = self.options["celix"].build_celix_etcdlib
        tc.cache_variables["TEST_LAUNCHER"] = self.options["celix"].build_launcher
        tc.cache_variables["TEST_PROMISES"] = self.options["celix"].build_promises
        tc.cache_variables["TEST_PUSHSTREAMS"] = self.options["celix"].build_pushstreams
        tc.cache_variables["TEST_LOG_HELPER"] = self.options["celix"].build_log_helper
        tc.cache_variables["TEST_LOG_SERVICE_API"] = self.options["celix"].build_log_service_api
        tc.cache_variables["TEST_CXX_REMOTE_SERVICE_ADMIN"] = self.options["celix"].build_cxx_remote_service_admin
        tc.cache_variables["TEST_SHELL_API"] = self.options["celix"].build_shell_api
        tc.cache_variables["TEST_CELIX_DFI"] = self.options["celix"].build_celix_dfi
        tc.cache_variables["TEST_UTILS"] = self.options["celix"].build_utils
        tc.cache_variables["TEST_COMPONENTS_READY_CHECK"] = self.options["celix"].build_components_ready_check
        # the following is workaround https://github.com/conan-io/conan/issues/7192
        for dep in self.dependencies.host.values():
            for ld in dep.cpp_info.libdirs:
                copy(self, "*", ld, os.path.join(self.build_folder, "lib"))
        tc.cache_variables["CMAKE_BUILD_RPATH"] = os.path.join(self.build_folder, "lib")
        tc.user_presets_path = False
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if not tools.cross_building(self, skip_x64_x86=True):
            if self.options["celix"].build_framework:
                self.run("./use_framework", run_environment=True)
            if self.options["celix"].build_http_admin:
                self.run("./use_http_admin", cwd=os.path.join("deploy", "use_http_admin"), run_environment=True)
            if self.options["celix"].build_log_service:
                self.run("./use_log_writer", cwd=os.path.join("deploy", "use_log_writer"), run_environment=True)
            if self.options["celix"].build_syslog_writer:
                self.run("./use_syslog_writer", cwd=os.path.join("deploy", "use_syslog_writer"), run_environment=True)
            if self.options["celix"].build_remote_service_admin:
                self.run("./use_my_rsa", cwd=os.path.join("deploy", "use_my_rsa"), run_environment=True)
                self.run("./use_c_rsa_spi", run_environment=True)
            if self.options["celix"].build_rsa_remote_service_admin_dfi and self.options["celix"].build_launcher:
                self.run("./use_rsa_dfi", cwd=os.path.join("deploy", "use_rsa_dfi"), run_environment=True)
            if self.options["celix"].build_rsa_remote_service_admin_shm_v2:
                self.run("./use_rsa_shm_v2", cwd=os.path.join("deploy", "use_rsa_shm_v2"), run_environment=True)
            if self.options["celix"].build_rsa_json_rpc:
                self.run("./use_rsa_rpc_json", cwd=os.path.join("deploy", "use_rsa_rpc_json"), run_environment=True)
            if self.options["celix"].build_rsa_discovery_configured and self.options["celix"].build_launcher:
                self.run("./use_rsa_configured", cwd=os.path.join("deploy", "use_rsa_configured"), run_environment=True)
            if self.options["celix"].build_rsa_discovery_etcd and self.options["celix"].build_launcher:
                self.run("./use_rsa_etcd", cwd=os.path.join("deploy", "use_rsa_etcd"), run_environment=True)
            if self.options["celix"].build_rsa_discovery_zeroconf:
                self.run("./use_rsa_discovery_zeroconf",
                         cwd=os.path.join("deploy", "use_rsa_discovery_zeroconf"), run_environment=True)
            if self.options["celix"].build_shell:
                self.run("./use_shell", run_environment=True)
                if self.options["celix"].celix_cxx17 or self.options["celix"].celix_cxx14:
                    self.run("./use_cxx_shell", run_environment=True)
            if self.options["celix"].build_remote_shell:
                self.run("./use_remote_shell", cwd=os.path.join("deploy", "use_remote_shell"), run_environment=True)
            if self.options["celix"].build_shell_tui:
                self.run("./use_shell_tui", cwd=os.path.join("deploy", "use_shell_tui"), run_environment=True)
            if self.options["celix"].build_shell_wui:
                self.run("./use_shell_wui", cwd=os.path.join("deploy", "use_shell_wui"), run_environment=True)
            if self.options["celix"].build_celix_etcdlib:
                self.run("./use_etcd_lib", run_environment=True)
            if self.options["celix"].build_launcher:
                self.run("./use_launcher", cwd=os.path.join("deploy", "use_launcher"), run_environment=True)
            if self.options["celix"].build_promises:
                self.run("./use_promises", run_environment=True)
            if self.options["celix"].build_pushstreams:
                self.run("./use_pushstreams", run_environment=True)
            if self.options["celix"].build_log_helper:
                self.run("./use_log_helper", run_environment=True)
            if self.options["celix"].build_log_service_api:
                self.run("./use_log_service_api", run_environment=True)
            if self.options["celix"].build_cxx_remote_service_admin:
                self.run("./use_cxx_remote_service_admin",
                         cwd=os.path.join("deploy", "use_cxx_remote_service_admin"), run_environment=True)
                self.run("./use_rsa_spi", run_environment=True)
            if self.options["celix"].build_shell_api:
                self.run("./use_shell_api", run_environment=True)
            if self.options["celix"].build_celix_dfi:
                self.run("./use_celix_dfi", run_environment=True)
            if self.options["celix"].build_utils:
                self.run("./use_utils", run_environment=True)
            if self.options["celix"].build_components_ready_check:
                self.run("./use_components_ready_check",
                         cwd=os.path.join("deploy", "use_components_ready_check"), run_environment=True)