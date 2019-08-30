#!/usr/bin/env bash
BUILD_SUPER_DIR=$1
DEPENDENCIES_DIR=$BUILD_SUPER_DIR/dogbox_dependencies

BOOST_VERSION_MINOR=71
BOOST_VERSION_PATCH=0
BOOST_VERSION=1.$BOOST_VERSION_MINOR.$BOOST_VERSION_PATCH
BOOST_VERSION_UNDERSCORES=1_${BOOST_VERSION_MINOR}_$BOOST_VERSION_PATCH
BENCHMARK_VERSION=v1.5.0
SQLITE3_VERSION=3290000
CMAKE_VERSION=3.15.2
GSL_VERSION=b576cc6ce375cf42f6537d65a9ef29d67aa6b78e

sudo apt install ninja-build g++ tar git wget clang-format-3.9 cmake libfuse-dev libssl-dev || exit 1
mkdir -p $DEPENDENCIES_DIR || exit 1
cd $DEPENDENCIES_DIR || exit 1

INSTALL_PREFIX=`pwd`/install.temp
rm -rf $INSTALL_PREFIX

# Boost
if [[ ! -d boost_$BOOST_VERSION_UNDERSCORES ]]; then
    wget https://dl.bintray.com/boostorg/release/$BOOST_VERSION/source/boost_$BOOST_VERSION_UNDERSCORES.tar.bz2 || exit 1
    tar xjf boost_$BOOST_VERSION_UNDERSCORES.tar.bz2 || exit 1
fi
pushd boost_$BOOST_VERSION_UNDERSCORES || exit 1
./bootstrap.sh || exit 1
./b2 --with-atomic --with-chrono --with-exception --with-filesystem --with-system --with-thread --with-test --prefix=$INSTALL_PREFIX link=static cxxflags=-std=c++17 install || exit 1
popd || exit 1

# GSL
if [[ ! -d GSL ]]; then
    git clone https://github.com/microsoft/GSL.git || exit 1
    pushd GSL || exit 1
else
    pushd GSL || exit 1
    git fetch || exit 1
fi
git checkout ${GSL_VERSION} || exit 1
popd || exit 1
mkdir -p GSL_build || exit 1
pushd GSL_build || exit 1
cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX -DGSL_CXX_STANDARD=17 -DGSL_TEST=OFF -G "Ninja" ../GSL || exit 1
cmake --build . --target install || exit 1
popd || exit 1

# SQLite3
if [[ ! -d sqlite-autoconf-$SQLITE3_VERSION ]]; then
    wget https://www.sqlite.org/2019/sqlite-autoconf-$SQLITE3_VERSION.tar.gz || exit 1
    tar zxvf sqlite-autoconf-$SQLITE3_VERSION.tar.gz || exit 1
fi
pushd sqlite-autoconf-$SQLITE3_VERSION || exit 1
./configure --prefix=$INSTALL_PREFIX --enable-static || exit 1
make -j8 install || exit 1
popd || exit 1

# Google Benchmark
if [[ ! -d benchmark ]]; then
    git clone https://github.com/google/benchmark.git || exit 1
    pushd benchmark || exit 1
else
    pushd benchmark || exit 1
    git fetch || exit 1
fi
git checkout ${BENCHMARK_VERSION} || exit 1
popd || exit 1
mkdir -p benchmark_build || exit 1
pushd benchmark_build || exit 1
cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_TESTING=OFF ../benchmark || exit 1
cmake --build . --target install || exit 1
popd || exit 1

# CMake
if [[ ! -d cmake-$CMAKE_VERSION ]]; then
    wget https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION.tar.gz || exit 1
    tar zxvf cmake-$CMAKE_VERSION.tar.gz || exit 1
fi
pushd cmake-$CMAKE_VERSION || exit 1
./bootstrap --prefix=$INSTALL_PREFIX || exit 1
make -j8 install || exit 1
popd || exit 1

rm -rf install
mv install.temp install || exit 1
