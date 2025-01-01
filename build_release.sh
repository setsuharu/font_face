#!/usr/bin/env bash

cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cd ..