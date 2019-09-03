# how to build

* only Ubuntu is supported at the moment
* git submodule init && git submodule update && ./install_dependencies.sh
  * a few Ubuntu packages will be installed
* use a recent version of CMake or the version installed by vcpkg
  * vcpkg/downloads/tools/cmake-*/cmake-*/bin/cmake
* GCC 8 or later is required
  * call CMake with -DCMAKE_CXX_COMPILER=g++-8
* recommended build tool: ninja
  * call CMake with -G "Ninja"
