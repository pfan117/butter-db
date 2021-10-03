#!/bin/bash

make clean
make GCC=gcc debug=1 install -j || exit 1

# eos
