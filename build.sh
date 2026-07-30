#!/bin/bash

SRC="src/main.cpp"

clang++ \
    -g -std=c++23 \
    -Wall -Wextra \
    $SRC \
    -o bin/raytray
