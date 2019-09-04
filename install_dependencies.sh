#!/usr/bin/env bash

BUILD_SUPER_DIR=$1
DEPENDENCIES_DIR=$BUILD_SUPER_DIR/dogbox_dependencies

sudo apt install ninja-build g++-8 clang-format-3.9 libfuse-dev || exit 1

mkdir -p $DEPENDENCIES_DIR || exit 1
pushd $DEPENDENCIES_DIR || exit 1

INSTALL_PREFIX=`pwd`/install.temp
rm -rf $INSTALL_PREFIX

# lcov
LCOV_VERSION=1.14
if [[ ! -d lcov-$LCOV_VERSION ]]; then
    wget https://github.com/linux-test-project/lcov/releases/download/v$LCOV_VERSION/lcov-$LCOV_VERSION.tar.gz || exit 1
    tar zxvf lcov-$LCOV_VERSION.tar.gz || exit 1
fi
pushd lcov-$LCOV_VERSION || exit 1
make -j4 PREFIX=$INSTALL_PREFIX install || exit 1
popd || exit 1

rm -rf install
mv install.temp install || exit 1

popd || exit 1

pushd vcpkg || exit 1
export CC=gcc-8
export CXX=g++-8
./bootstrap-vcpkg.sh || exit 1
./vcpkg install sqlite3 boost-system boost-test boost-outcome boost-filesystem boost-process ms-gsl benchmark lz4 || exit 1
