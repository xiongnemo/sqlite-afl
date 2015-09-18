#!/bin/sh
#
# Run this script to rebuild the program used for fuzzing.
#
OBJ="fuzzershell"
SRC="sqlite3.c fuzzershell.c"
FILES="$SRC sqlite3.h"
ls -l $FILES
sha1sum $FILES
echo "Building from the files above...."
OPTS="$OPTS -DSQLITE_THREADSAFE=0"
OPTS="$OPTS -DSQLITE_ENABLE_LOAD_EXTENSION=0"
OPTS="$OPTS -DSQLITE_NO_SYNC=0"
OPTS="$OPTS -DSQLITE_SQLITE_DEBUG=0"
OPTS="$OPTS -DSQLITE_OMIT_RANDOMNESS=0"
OPTS="$OPTS -DSQLITE_ENABLE_JSON1=0"
../afl-gcc -O3 -o $OBJ $OPTS -I. $SRC -ldl
ls -l $OBJ
