#!/bin/bash
make distclean
./autogen.sh
./configure
make
