#!/bin/bash

set -ev
set -x

cd 3rdparty
unzip gtest-1.7.0.zip
cd ..
mkdir -p  build_travis
cd build_travis
cmake  ..
cmake --build .
./UnitTestRunner
