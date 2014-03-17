#!/bin/sh
./autogen.sh
PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./configure
make clean
make -j6
