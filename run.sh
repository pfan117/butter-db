#!/bin/bash

TMPDATAPATH="/var/tmp/butter-db-test/"

mkdir -p ${TMPDATAPATH} || exit 1
rm -f ${TMPDATAPATH}/*

# make clean

if [[ -f tst/create-tc-list.sh ]] ; then
	:
else
	tst/create-tc-list.sh
fi

make GCC=gcc test_mode=1 TMPDATAPATH=${TMPDATAPATH} test || exit 1

ctags -R *

./a.out --mt
#./a.out --mt --tc 43

rm -rf coverage
mkdir coverage || exit 1

lcov  -q -d . -b . -c -o coverage.info.txt || exit 1

genhtml -q -o coverage coverage.info.txt || exit 1

# eos
