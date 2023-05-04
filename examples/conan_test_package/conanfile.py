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

from conans import CMake, ConanFile, tools
import os


class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "cmake_paths", "cmake_find_package"

    def build(self):
        cmake = CMake(self)
        cmake.definitions["TEST_HTTP_ADMIN"] = self.options["celix"].build_http_admin
        cmake.definitions["TEST_LOG_SERVICE"] = self.options["celix"].build_log_service
        cmake.definitions["TEST_SYSLOG_WRITER"] = self.options["celix"].build_syslog_writer
        cmake.definitions["TEST_PUBSUB"] = self.options["celix"].build_pubsub
        cmake.definitions["TEST_PSA_ZMQ"] = self.options["celix"].build_pubsub_psa_zmq
        cmake.definitions["TEST_PSA_TCP"] = self.options["celix"].build_pubsub_psa_tcp
        cmake.definitions["TEST_PSA_UDP_MC"] = self.options["celix"].build_pubsub_psa_udp_mc
        cmake.definitions["TEST_PSA_WS"] = self.options["celix"].build_pubsub_psa_ws
        cmake.definitions["TEST_PSA_DISCOVERY_ETCD"] = self.options["celix"].build_pubsub_discovery_etcd
        cmake.definitions["TEST_RSA"] = self.options["celix"].build_remote_service_admin
        cmake.definitions["TEST_RSA_DFI"] = self.options["celix"].build_rsa_remote_service_admin_dfi
        cmake.definitions["TEST_RSA_SHM_V2"] = self.options["celix"].build_rsa_remote_service_admin_shm_v2
        cmake.definitions["TEST_RSA_RPC_JSON"] = self.options["celix"].build_rsa_json_rpc
        cmake.definitions["TEST_RSA_DISCOVERY_CONFIGURED"] = self.options["celix"].build_rsa_discovery_configured
        cmake.definitions["TEST_RSA_DISCOVERY_ETCD"] = self.options["celix"].build_rsa_discovery_etcd
        cmake.definitions["TEST_RSA_DISCOVERY_ZEROCONF"] = self.options["celix"].build_rsa_discovery_zeroconf
        cmake.definitions["TEST_SHELL"] = self.options["celix"].build_shell
        if self.options["celix"].build_shell:
            cmake.definitions["TEST_CXX_SHELL"] = self.options["celix"].celix_cxx17 or self.options["celix"].celix_cxx14
        cmake.definitions["TEST_REMOTE_SHELL"] = self.options["celix"].build_remote_shell
        cmake.definitions["TEST_SHELL_TUI"] = self.options["celix"].build_shell_tui
        cmake.definitions["TEST_SHELL_WUI"] = self.options["celix"].build_shell_wui
        cmake.definitions["TEST_ETCD_LIB"] = self.options["celix"].build_celix_etcdlib
        cmake.definitions["TEST_LAUNCHER"] = self.options["celix"].build_launcher
        cmake.definitions["TEST_PROMISES"] = self.options["celix"].build_promises
        cmake.definitions["TEST_PUSHSTREAMS"] = self.options["celix"].build_pushstreams
        cmake.definitions["CMAKE_PROJECT_test_package_INCLUDE"] = os.path.join(self.build_folder, "conan_paths.cmake")
        # the following is workaround https://github.com/conan-io/conan/issues/7192
        if self.settings.os == "Linux":
            cmake.definitions["CMAKE_EXE_LINKER_FLAGS"] = "-Wl,--unresolved-symbols=ignore-in-shared-libs"
        elif self.settings.os == "Macos":
            cmake.definitions["CMAKE_EXE_LINKER_FLAGS"] = "-Wl,-undefined -Wl,dynamic_lookup"
        cmake.configure()
        cmake.build()

    def test(self):
        if not tools.cross_building(self, skip_x64_x86=True):
            self.run("./use_framework", run_environment=True)
            if self.options["celix"].build_http_admin:
                self.run("./use_http_admin", cwd=os.path.join("deploy", "use_http_admin"), run_environment=True)
            if self.options["celix"].build_log_service:
                self.run("./use_log_writer", cwd=os.path.join("deploy", "use_log_writer"), run_environment=True)
            if self.options["celix"].build_syslog_writer:
                self.run("./use_syslog_writer", cwd=os.path.join("deploy", "use_syslog_writer"), run_environment=True)
            if self.options["celix"].build_pubsub:
                self.run("./use_my_psa", cwd=os.path.join("deploy", "use_my_psa"), run_environment=True)
            if self.options["celix"].build_pubsub_psa_zmq:
                self.run("./use_psa_zmq", cwd=os.path.join("deploy", "use_psa_zmq"), run_environment=True)
            if self.options["celix"].build_pubsub_psa_tcp:
                self.run("./use_psa_tcp", cwd=os.path.join("deploy", "use_psa_tcp"), run_environment=True)
            if self.options["celix"].build_pubsub_psa_udp_mc:
                self.run("./use_psa_udp_mc", cwd=os.path.join("deploy", "use_psa_udp_mc"), run_environment=True)
            if self.options["celix"].build_pubsub_psa_ws:
                self.run("./use_psa_ws", cwd=os.path.join("deploy", "use_psa_ws"), run_environment=True)
            if self.options["celix"].build_pubsub_discovery_etcd:
                self.run("./use_psa_discovery_etcd", cwd=os.path.join("deploy", "use_psa_discovery_etcd"), run_environment=True)
            if self.options["celix"].build_remote_service_admin:
                self.run("./use_my_rsa", cwd=os.path.join("deploy", "use_my_rsa"), run_environment=True)
                self.run("./use_c_rsa_spi", run_environment=True)
            if self.options["celix"].build_rsa_remote_service_admin_dfi:
                self.run("./use_rsa_dfi", cwd=os.path.join("deploy", "use_rsa_dfi"), run_environment=True)
            if self.options["celix"].build_rsa_remote_service_admin_shm_v2:
                self.run("./use_rsa_shm_v2", cwd=os.path.join("deploy", "use_rsa_shm_v2"), run_environment=True)
            if self.options["celix"].build_rsa_json_rpc:
                self.run("./use_rsa_rpc_json", cwd=os.path.join("deploy", "use_rsa_rpc_json"), run_environment=True)
            if self.options["celix"].build_rsa_discovery_configured:
                self.run("./use_rsa_configured", cwd=os.path.join("deploy", "use_rsa_configured"), run_environment=True)
            if self.options["celix"].build_rsa_discovery_etcd:
                self.run("./use_rsa_etcd", cwd=os.path.join("deploy", "use_rsa_etcd"), run_environment=True)
            if self.options["celix"].build_rsa_discovery_zeroconf:
                self.run("./use_rsa_discovery_zeroconf", cwd=os.path.join("deploy", "use_rsa_discovery_zeroconf"), run_environment=True)
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
