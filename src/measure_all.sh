#!/bin/bash

set -e
shopt -s extglob

for exe in ../build/src/lab3*
do
    echo "Measuring $exe...";
    ./measure.sh $exe 900 25000;
done
