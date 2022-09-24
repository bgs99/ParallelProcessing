#!/bin/bash

set -e

res_1=$($1 1000 | grep 'X=')
res_2=$($2 1000 | grep 'X=')

if [ "$res_1" != "$res_2" ]
then
    echo "Failed:";
    echo "$res_1";
    echo "";
    echo "vs";
    echo "";
    echo "$res_2";
fi

test "$res_1" = "$res_2"
