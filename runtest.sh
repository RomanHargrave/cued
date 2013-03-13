#!/bin/sh
export MALLOC_TRACE=/tmp/mtrace-cued
export TEST_PROG=src/test/object
${TEST_PROG}
mtrace ${TEST_PROG} ${MALLOC_TRACE}
