from conans import ConanFile, CMake


class LightningProject(ConanFile):
    name = "Lightning"
    version = "0.0.1"
    license = "MIT"
    author = "Nathaniel Rupprecht"
    url = "https://github.com/nrupprecht/Lightning"
    description = "Lightning logging"
    topics = ("conan", "lightning", "logging")  # Specify relevant topics or keywords
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "include/**", "source/**", "CMakeLists.txt", "cmake/**", "test/**"

    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv", "cmake"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def layout(self):
        self.folders.generators = "conan"

    def package(self):
        include_folder = "include"
        self.copy("*.h", dst="include", src=include_folder)
        self.copy("*.hpp", dst="include", src=include_folder)
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["lightning"]

    def build_requirements(self):
        self.test_requires("gtest/1.11.0")
