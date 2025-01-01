#!/usr/bin/env bash

cd build_debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
cd ..