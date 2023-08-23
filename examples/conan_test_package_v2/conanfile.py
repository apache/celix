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
        tc.cache_variables["TEST_PUBSUB"] = celix_options.build_pubsub
        tc.cache_variables["TEST_PSA_ZMQ"] = celix_options.build_pubsub_psa_zmq
        tc.cache_variables["TEST_PSA_TCP"] = celix_options.build_pubsub_psa_tcp
        tc.cache_variables["TEST_PSA_UDP_MC"] = celix_options.build_pubsub_psa_udp_mc
        tc.cache_variables["TEST_PSA_WS"] = celix_options.build_pubsub_psa_ws
        tc.cache_variables["TEST_PSA_DISCOVERY_ETCD"] = celix_options.build_pubsub_discovery_etcd
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
        tc.cache_variables["TEST_DEPLOYMENT_ADMIN"] = celix_options.build_deployment_admin
        tc.cache_variables["TEST_LOG_HELPER"] = celix_options.build_log_helper
        tc.cache_variables["TEST_LOG_SERVICE_API"] = celix_options.build_log_service_api
        tc.cache_variables["TEST_PUBSUB_WIRE_PROTOCOL_V1"] = celix_options.build_pubsub_wire_protocol_v1
        tc.cache_variables["TEST_PUBSUB_WIRE_PROTOCOL_V2"] = celix_options.build_pubsub_wire_protocol_v2
        tc.cache_variables["TEST_PUBSUB_JSON_SERIALIZER"] = celix_options.build_pubsub_json_serializer
        tc.cache_variables["TEST_PUBSUB_AVROBIN_SERIALIZER"] = celix_options.build_pubsub_avrobin_serializer
        tc.cache_variables["TEST_CXX_REMOTE_SERVICE_ADMIN"] = celix_options.build_cxx_remote_service_admin
        tc.cache_variables["TEST_SHELL_API"] = celix_options.build_shell_api
        tc.cache_variables["TEST_SHELL_BONJOUR"] = celix_options.build_shell_bonjour
        tc.cache_variables["TEST_CELIX_DFI"] = celix_options.build_celix_dfi
        tc.cache_variables["TEST_UTILS"] = celix_options.build_utils
        tc.cache_variables["TEST_COMPONENTS_READY_CHECK"] = celix_options.build_components_ready_check
        tc.user_presets_path = False
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            celix_options = self.dependencies["celix"].options
            if celix_options.build_framework:
                self.run("./use_framework", run_environment=True)
            if celix_options.build_http_admin:
                self.run("./use_http_admin", cwd=os.path.join("deploy", "use_http_admin"), run_environment=True)
            if celix_options.build_log_service:
                self.run("./use_log_writer", cwd=os.path.join("deploy", "use_log_writer"), run_environment=True)
            if celix_options.build_syslog_writer:
                self.run("./use_syslog_writer", cwd=os.path.join("deploy", "use_syslog_writer"), run_environment=True)
            if celix_options.build_pubsub:
                self.run("./use_my_psa", cwd=os.path.join("deploy", "use_my_psa"), run_environment=True)
            if celix_options.build_pubsub_psa_zmq:
                self.run("./use_psa_zmq", cwd=os.path.join("deploy", "use_psa_zmq"), run_environment=True)
            if celix_options.build_pubsub_psa_tcp:
                self.run("./use_psa_tcp", cwd=os.path.join("deploy", "use_psa_tcp"), run_environment=True)
            if celix_options.build_pubsub_psa_udp_mc:
                self.run("./use_psa_udp_mc", cwd=os.path.join("deploy", "use_psa_udp_mc"), run_environment=True)
            if celix_options.build_pubsub_psa_ws:
                self.run("./use_psa_ws", cwd=os.path.join("deploy", "use_psa_ws"), run_environment=True)
            if celix_options.build_pubsub_discovery_etcd and celix_options.build_launcher:
                self.run("./use_psa_discovery_etcd",
                         cwd=os.path.join("deploy", "use_psa_discovery_etcd"), run_environment=True)
            if celix_options.build_remote_service_admin:
                self.run("./use_my_rsa", cwd=os.path.join("deploy", "use_my_rsa"), run_environment=True)
                self.run("./use_c_rsa_spi", run_environment=True)
            if celix_options.build_rsa_remote_service_admin_dfi and celix_options.build_launcher:
                self.run("./use_rsa_dfi", cwd=os.path.join("deploy", "use_rsa_dfi"), run_environment=True)
            if celix_options.build_rsa_remote_service_admin_shm_v2:
                self.run("./use_rsa_shm_v2", cwd=os.path.join("deploy", "use_rsa_shm_v2"), run_environment=True)
            if celix_options.build_rsa_json_rpc:
                self.run("./use_rsa_rpc_json", cwd=os.path.join("deploy", "use_rsa_rpc_json"), run_environment=True)
            if celix_options.build_rsa_discovery_configured and celix_options.build_launcher:
                self.run("./use_rsa_configured", cwd=os.path.join("deploy", "use_rsa_configured"), run_environment=True)
            if celix_options.build_rsa_discovery_etcd and celix_options.build_launcher:
                self.run("./use_rsa_etcd", cwd=os.path.join("deploy", "use_rsa_etcd"), run_environment=True)
            if celix_options.build_rsa_discovery_zeroconf:
                self.run("./use_rsa_discovery_zeroconf",
                         cwd=os.path.join("deploy", "use_rsa_discovery_zeroconf"), run_environment=True)
            if celix_options.build_shell:
                self.run("./use_shell", run_environment=True)
                if celix_options.celix_cxx17 or celix_options.celix_cxx14:
                    self.run("./use_cxx_shell", run_environment=True)
            if celix_options.build_remote_shell:
                self.run("./use_remote_shell", cwd=os.path.join("deploy", "use_remote_shell"), run_environment=True)
            if celix_options.build_shell_tui:
                self.run("./use_shell_tui", cwd=os.path.join("deploy", "use_shell_tui"), run_environment=True)
            if celix_options.build_shell_wui:
                self.run("./use_shell_wui", cwd=os.path.join("deploy", "use_shell_wui"), run_environment=True)
            if celix_options.build_celix_etcdlib:
                self.run("./use_etcd_lib", run_environment=True)
            if celix_options.build_launcher:
                self.run("./use_launcher", cwd=os.path.join("deploy", "use_launcher"), run_environment=True)
            if celix_options.build_promises:
                self.run("./use_promises", run_environment=True)
            if celix_options.build_pushstreams:
                self.run("./use_pushstreams", run_environment=True)
            if celix_options.build_deployment_admin:
                self.run("./use_deployment_admin",
                         cwd=os.path.join("deploy", "use_deployment_admin"), run_environment=True)
            if celix_options.build_log_helper:
                self.run("./use_log_helper", run_environment=True)
            if celix_options.build_log_service_api:
                self.run("./use_log_service_api", run_environment=True)
            if celix_options.build_pubsub_wire_protocol_v1:
                self.run("./use_pubsub_wire_protocol_v1",
                         cwd=os.path.join("deploy", "use_pubsub_wire_protocol_v1"), run_environment=True)
            if celix_options.build_pubsub_wire_protocol_v2:
                self.run("./use_pubsub_wire_protocol_v2",
                         cwd=os.path.join("deploy", "use_pubsub_wire_protocol_v2"), run_environment=True)
            if celix_options.build_pubsub_json_serializer:
                self.run("./use_pubsub_json_serializer",
                         cwd=os.path.join("deploy", "use_pubsub_json_serializer"), run_environment=True)
            if celix_options.build_pubsub_avrobin_serializer:
                self.run("./use_pubsub_avrobin_serializer",
                         cwd=os.path.join("deploy", "use_pubsub_avrobin_serializer"), run_environment=True)
            if celix_options.build_cxx_remote_service_admin:
                self.run("./use_cxx_remote_service_admin",
                         cwd=os.path.join("deploy", "use_cxx_remote_service_admin"), run_environment=True)
                self.run("./use_rsa_spi", run_environment=True)
            if celix_options.build_shell_api:
                self.run("./use_shell_api", run_environment=True)
            if celix_options.build_shell_bonjour:
                self.run("./use_shell_bonjour",
                         cwd=os.path.join("deploy", "use_shell_bonjour"), run_environment=True)
            if celix_options.build_celix_dfi:
                self.run("./use_celix_dfi", run_environment=True)
            if celix_options.build_utils:
                self.run("./use_utils", run_environment=True)
            if celix_options.build_components_ready_check:
                self.run("./use_components_ready_check",
                         cwd=os.path.join("deploy", "use_components_ready_check"), run_environment=True)
