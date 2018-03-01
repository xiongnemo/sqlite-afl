#!/bin/sh
#
# Run this to recompile for session fuzzing.
#
../afl-gcc -O3 -g -o sessionfuzz sessionfuzz.c
ls -l sessionfuzz
