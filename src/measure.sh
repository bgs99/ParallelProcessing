#!/bin/bash

set -e

base=$1;
min=$2;
max=$3;

shift 3;

pars="1 2 4 6 8";

step=$(((max - min) / 10));

echo "Step is $step";

export LD_LIBRARY_PATH='/home/$ENV{USER}/Downloads/framewave/lib'

function process {
    par=$1;
    bin="${base} ${par}";
    csv="${base}-${par}.csv";
    echo "Measuring $bin";
    rm -f ${csv};
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
        echo "$n;$mindelta" >> ${csv};
        echo "$n;$mindelta";
    done
}

for par in ${pars}
do
    process $par;
done
