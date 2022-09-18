#!/bin/bash

set -e

test "$($1 1000 | grep 'X=')" = "$($2 1000 | grep 'X=')"
