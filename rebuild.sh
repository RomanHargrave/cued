#!/bin/sh
./autogen.sh
./configure
make clean
make -j6
