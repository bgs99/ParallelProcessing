#!/bin/bash

set -e
./measure.sh ../build/lab1-gcc 900 25000
./measure.sh ../build/lab1-icc 1500 50000
./measure.sh ../build/lab1-solaris 600 14500
