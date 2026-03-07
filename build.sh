#!/bin/sh
clear
cd /home/rikka/study/reactor
rm -rf build
mkdir build
cmake --preset=vcpkg
cmake --build build