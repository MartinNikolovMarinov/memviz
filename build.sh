#!/bin/bash

export CC=clang
export CXX=clang++

cmake -S . -B build -G Ninja
cmake --build build
