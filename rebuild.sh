#!/bin/sh
./autogen.sh
./configure --enable-classc
make clean
make -j6
