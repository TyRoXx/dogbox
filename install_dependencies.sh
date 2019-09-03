#!/usr/bin/env bash

sudo apt install ninja-build g++-8 clang-format-3.9 libfuse-dev || exit 1

pushd vcpkg || exit 1
export CC=gcc-8
export CXX=g++-8
./bootstrap-vcpkg.sh || exit 1
./vcpkg install sqlite3 boost-system boost-test boost-outcome boost-filesystem boost-process ms-gsl benchmark || exit 1
