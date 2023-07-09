from   conans       import ConanFile, CMake, tools
from   conans.tools import download, unzip
import os

class Project(ConanFile):
    name            = "Lightning"
    description     = "Lightning logging"
    version         = "0.0.1"
    url             = "https://github.com/nrupprecht/Lightning.git"
    settings        = "arch", "build_type", "compiler", "os"
    generators      = "cmake"

    options         = {"testing": [True, False]}
    default_options = {"testing": True}

    def imports(self):
        self.copy("*.dylib*", dst="", src="lib")
        self.copy("*.dll"   , dst="", src="bin")

    def source(self):
        pass

    def requirements(self):
        """ List all required conan packages here. """
        self.requires("gtest/1.11.0")

    def build(self):
        cmake          = CMake(self)
        cmake.definitions["BUILD_TESTS"] = self.options.testing

        cmake.configure()
        cmake.build()
        if self.options.testing:
            cmake.test()
        cmake.install()

    def package(self):
        include_folder = "%s-%s/include" % (self.name, self.version)
        self.copy("*.h"     , dst="include", src=include_folder)
        self.copy("*.hpp"   , dst="include", src=include_folder)
        self.copy("*.inl"   , dst="include", src=include_folder)
        self.copy("*.dylib*", dst="lib"    , keep_path=False   )
        self.copy("*.lib"   , dst="lib"    , keep_path=False   )
        self.copy("*.so*"   , dst="lib"    , keep_path=False   )
        self.copy("*.dll"   , dst="bin"    , keep_path=False   )

    def package_info(self):
        self.cpp_info.libs = [self.name]
        if self.settings.os == "Windows":
            if not self.options.shared:
                self.cpp_info.defines.append("%s_STATIC" % self.name.upper())
