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
    generators = "cmake_paths"

    def requirements(self):
        # for test_package, `requires` is set by conan, and thus not needed
        # the following make the test package can be used as a standalone package consumer
        if not self.requires:
            self.requires("celix/2.2.3@zhengpeng/testing")

    def build(self):
        cmake = CMake(self)
        cmake.definitions["TEST_HTTP_ADMIN"] = self.options["celix"].build_http_admin
        cmake.definitions["TEST_LOG_SERVICE"] = self.options["celix"].build_log_service
        cmake.definitions["TEST_SYSLOG_WRITER"] = self.options["celix"].build_syslog_writer
        cmake.definitions["TEST_SHELL"] = self.options["celix"].build_shell
        cmake.definitions["TEST_REMOTE_SHELL"] = self.options["celix"].build_remote_shell
        cmake.definitions["TEST_SHELL_TUI"] = self.options["celix"].build_shell_tui
        cmake.definitions["TEST_SHELL_WUI"] = self.options["celix"].build_shell_wui
        cmake.definitions["CMAKE_PROJECT_test_package_INCLUDE"] = os.path.join(self.build_folder, "conan_paths.cmake")
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
            if self.options["celix"].build_shell:
                self.run("./use_shell", run_environment=True)
            if self.options["celix"].build_remote_shell:
                self.run("./use_remote_shell", cwd=os.path.join("deploy", "use_remote_shell"), run_environment=True)
            if self.options["celix"].build_shell_tui:
                self.run("./use_shell_tui", cwd=os.path.join("deploy", "use_shell_tui"), run_environment=True)
            if self.options["celix"].build_shell_wui:
                self.run("./use_shell_wui", cwd=os.path.join("deploy", "use_shell_wui"), run_environment=True)
