from conans import ConanFile, CMake, tools
from conans.errors import ConanException, ConanInvalidConfiguration
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
    # TODO: update according to latest codes
    options = {
        "enable_testing": [True, False],
        "enable_address_sanitizer": [True, False],
        "enable_undefined_sanitizer": [True, False],
        "enable_code_coverage": [True, False],
        "celix_add_openssl_dep": [True, False],
        "build_deployment_admin": [True, False],
        "build_device_access_example": [True, False],
        "build_device_access": [True, False],
        "build_http_admin": [True, False],
        "build_log_service": [True, False],
        "build_log_writer": [True, False],
        "build_log_writer_syslog": [True, False],
        "build_pubsub": [True, False],
        "build_pubsub_psa_zmq": [True, False],
        "build_zmq_security": [True, False],
        "build_pubsub_tests": [True, False],
        "build_remote_service_admin": [True, False],
        "build_rsa_remote_service_admin_dfi": [True, False],
        "build_rsa_discovery_configured": [True, False],
        "build_rsa_discovery_etcd": [True, False],
        "build_rsa_discovery_shm": [True, False],
        "build_rsa_topology_manager": [True, False],
        "build_rsa_examples": [True, False],
        "build_shell": [True, False],
        "build_remote_shell": [True, False],
        "build_shell_bonjour": [True, False],
        "build_shell_tui": [True, False],
        "build_shell_wui": [True, False],
        "build_examples": [True, False],
        "build_launcher": [True, False],
        "build_event_admin": [True, False],
        "build_experimental": [True, False],
        "celix_cxx": [True, False],
    }
    default_options = { 
        "enable_testing": False,
        "enable_address_sanitizer": False,
        "enable_undefined_sanitizer": False,
        "enable_code_coverage": False,
        "celix_add_openssl_dep": False,
        "build_deployment_admin": False,
        "build_device_access": True,
        "build_device_access_example": False,
        "build_http_admin": True,
        "build_log_service": True,
        "build_log_writer": True,
        "build_log_writer_syslog": True,
        "build_pubsub": True,
        "build_pubsub_psa_zmq": False,
        "build_zmq_security": False,
        "build_pubsub_tests": False,
        "build_remote_service_admin": True,
        "build_rsa_remote_service_admin_dfi": True,
        "build_rsa_discovery_configured": True,
        "build_rsa_discovery_etcd": True,
        "build_rsa_discovery_shm": False,
        "build_rsa_topology_manager": True,
        "build_rsa_examples": True,
        "build_shell": True,
        "build_remote_shell": True,
        "build_shell_bonjour": False,
        "build_shell_tui": True,
        "build_shell_wui": False,
        "build_examples": True,
        "build_launcher": True,
        "build_event_admin": False,
        "build_experimental": False,
        "celix_cxx": False,
    }
    _cmake = None

    def validate(self):
        if self.settings.os != "Linux" and self.settings.os != "Macos":
            raise ConanInvalidConfiguration("Library MyLib is only supported for Linux")

    def configure(self):
        if not self.options.enable_testing:
            self.options.build_pubsub_tests = False;
        if not self.options.build_device_access:
            self.options.build_device_access_example = False
        if not self.options.build_log_writer:
            self.options.build_log_writer_syslog = False
        if not self.options.build_pubsub:
            self.options.build_pubsub_psa_zmq = False
        if not self.options.build_pubsub_psa_zmq:
            self.options.build_zmq_security = False
        if not self.options.build_remote_service_admin:
            self.options.build_rsa_remote_service_admin_dfi = False
            self.options.build_rsa_discovery_configured = False
            self.options.build_rsa_discovery_etcd = False
            self.options.build_rsa_discovery_shm = False
            self.options.build_rsa_topology_manager = False
            self.options.build_rsa_examples = False
        if not self.options.build_shell:
            self.options.build_remote_shell = False
            self.options.build_shell_bonjour = False
            self.options.build_shell_tui = False
            self.options.build_shell_wui = False
        if not self.options.build_experimental:
            self.options.build_event_admin = False

    def requirements(self):
        # libffi/3.3@zhengpeng/testing is a workaround of the following buggy commit:
        # https://github.com/conan-io/conan-center-index/pull/5085#issuecomment-847487808
        #self.requires("libffi/3.3@zhengpeng/testing")
        self.requires("libffi/[~3.2.1]")
        self.requires("jansson/[~2.12]")
        self.requires("libcurl/[~7.64.1]")
        self.requires("zlib/[~1.2.8]")
        self.requires("libuuid/1.0.3")
        self.requires("libzip/1.8.0")
        self.options['libffi'].shared = True
        self.options['jansson'].shared = True
        self.options['libcurl'].shared = True
        self.options['zlib'].shared = True
        self.options['libuuid'].shared = True
        self.options['libzip'].shared = True
        self.options['openssl'].shared = True
        self.options['libxml2'].shared = True
        if self.options.enable_testing:
            self.requires("gtest/1.10.0")
            self.requires("cpputest/4.0")
        if self.options.celix_add_openssl_dep or self.options.build_zmq_security:
            self.requires("openssl/1.1.1k")
        if self.options.build_remote_service_admin or self.options.build_shell_bonjour:
            self.requires("libxml2/[~2.9.9]")
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
        self.output.info(self._cmake.definitions)
        self._cmake.configure()
        return self._cmake

    def build(self):
        # self._patch_sources()
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy("LICENSE", dst="licenses", src=self.source_folder)
        cmake = self._configure_cmake()
        cmake.install()
        # tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        self.cpp_info.builddirs = [os.path.join("share", self.name, "cmake")];
        self.cpp_info.bindirs = ["bin", os.path.join("share", self.name, "bundles")]
        self.cpp_info.build_modules["cmake"].append(os.path.join("share", self.name, "cmake", "cmake_celix", "UseCelix.cmake"))
        self.cpp_info.build_modules["cmake"].append(os.path.join("share", self.name, "cmake", "Targets.cmake"))
        self.cpp_info.build_modules["cmake"].append(os.path.join("share", self.name, "cmake", "CelixTargets.cmake"))
