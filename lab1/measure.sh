#!/bin/bash

set -e

base=$1;
min=$2;
max=$3;

shift 3;

pars="1 2 4 8 10";

step=$(((max - min) / 10));

echo "Step is $step";

function process {
    bin="${base}-$1"
    echo "Measuring $bin";
    rm -f ${bin}.csv
    for i in {0..10}
    do
        n=$((min + i * step));
        delta=$($bin $n | sed -En 's/^.* ([0-9]+)$/\1/p');
        mindelta=$delta
        for j in {1..4}
        do
            delta=$($bin $n | sed -En 's/^.* ([0-9]+)$/\1/p');
            if [ $delta -le $mindelta ]
            then
                mindelta=$delta;
            fi
        done
        echo "$n;$mindelta" >> ${bin}.csv;
        echo "$n;$mindelta";
    done
}

process "seq";

for par in ${pars}
do
    process "par-$par";
done
