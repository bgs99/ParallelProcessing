#!/bin/bash

set -e

BIN_PATH=$1;
SEQ_RESULT=$($1/lab1-seq-verify 1000 | grep 'X=')
PAR_RESULT=$($1/lab1-par-verify 1000 | grep 'X=')
test "$SEQ_RESULT" = "$PAR_RESULT"
