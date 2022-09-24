#!/bin/bash

set -e
./measure.sh ../build/lab1-gcc 900 25000
./measure.sh ../build/lab1-clang 900 26800
./measure.sh ../build/lab1-rocm 900 26800
